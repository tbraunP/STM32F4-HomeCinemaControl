#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "tasks/SystemStateWatcher.h"

#include "irmp/irmp.h"
#include "tasks/SolidStateTask.h"
#include "tasks/IrmpTask.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tasks/Task_Priorities.h"
#include "util/linkedlist.h"


// Logging
#ifdef ENABLE_LOG_SYS
#define LOG_SYS_LOG(...)	printf( __VA_ARGS__ )
#define LOG_SYS_ERR(...) 	printf( __VA_ARGS__ )
#else
#define LOG_SYS_LOG(format, ...)
#define LOG_SYS_ERR(...)	printf(__VA_ARGS__ )
#endif


struct {
     linkedlist_t statusMessages;
     linkedlist_t connections;
     // exclusive access to statusMessage and connection container
     xSemaphoreHandle exclusiveDataAccess;

     xQueueHandle systemStateQueue;
} ssw_state;


/**
 * Process-Handler for foreach. This handler clean orphoned connections
 */
void SystemStateWatcher_CleanConnection ( linkedlist_node_t* node, void* param )
{
     ConnectionHandler_t* connection = ( ConnectionHandler_t* ) node->data;
     if ( connection->connectionBroken ) {
          xSemaphoreGive ( connection->connectionFreeSemaphore );
          // freeing the data is done by IncomingDataHandler
          node->data = NULL;
     }
}

/**
 * Compare two Status_Update_t and return true if fromComponent and uuid are equal.
 * \param in1 - first component
 * \param in2 - second component
 * \return true if both, key.fromComponent and key.uuid are equal
 */
bool SystemStateWatcher_SearchStatusUpdate ( void* in1, void* in2 )
{
     Status_Update_t* s1 = ( Status_Update_t* ) in1;
     Status_Update_t* s2 = ( Status_Update_t* ) in2;

     if ( ( s1->key.fromComponent == s2->key.fromComponent ) && ( s1->key.uuid == s2->key.uuid ) )
          return true;
     return false;
}


static void inline SystemStateWatcher_Transfer ( ConnectionHandler_t* connection, Status_Update_t* status )
{
     LOG_SYS_LOG ( "..Transfering state update\n" );
     // build and transmit frame
     size_t len = status->len + 8;
     uint8_t* data = malloc ( len );
     memcpy ( &data[6], status->payload.raw, status->len );

     // build header and append trailer
     data[0] = 0xAB;
     data[1] = 0xCD;
     data[2] = ( uint8_t ) len;
     data[3] = 0x02;
     data[4] = status->key.fromComponent;
     data[5] = 0xFF;
     data[len-2] = 0xEF;
     data[len-1] = 0xFE;

     // send frame to output queue
     PhysicalFrame_t phyFrame = {.len = len, .payload = data};
     if ( xQueueSend ( connection->connectionQueue, &phyFrame, ( portTickType ) 0 ) != pdTRUE ) {
          free ( data );
     }
}

/**
 * Process-Handler for foreach. This transfers a status update (given as param) using
 * the connection stored in node
 * \param node - ConnectionHandler_t
 * \param param - param contains the Status update
 */
void SystemStateWatcher_TransmitStatusUpdate ( linkedlist_node_t* node, void* param )
{
     Status_Update_t* status = ( Status_Update_t* ) param;
     ConnectionHandler_t* connection = ( ConnectionHandler_t* ) node->data;

     SystemStateWatcher_Transfer ( connection, status );
}


/**
 * Process-Handler for foreach. This transfers a status update (given as param) using
 * the connection stored in node
 * \param node - Status_Update_t
 * \param param - ConnectionHandler_t
 */
void SystemStateWatcher_TransmitStatusUpdate2 ( linkedlist_node_t* node, void* param )
{
     ConnectionHandler_t* connection = ( ConnectionHandler_t* ) param;
     Status_Update_t* status = ( Status_Update_t* ) node->data;

     SystemStateWatcher_Transfer ( connection, status );
}


/**
 * Propagate a status update
 * \param status - Status update to propagate
 */
void SystemStateWatcher_propagateUpdate ( Status_Update_t* status )
{
     linkedlist_foreach ( &ssw_state.connections, SystemStateWatcher_TransmitStatusUpdate, status );
}

/* Thread to Handle status updates */
void SystemStateWatcher_Task_thread()
{
     static Status_Update_t status;

     for ( ;; ) {
          if ( xQueuePeek ( ssw_state.systemStateQueue, & ( status ), ( portTickType ) ( 800/portTICK_RATE_MS ) )  == pdTRUE ) {
               if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) ( 50/portTICK_RATE_MS ) ) == pdTRUE ) {
                    // read message again and delete it from queue
                    if ( xQueueReceive ( ssw_state.systemStateQueue, & ( status ), ( portTickType ) ( 0 ) )  != pdTRUE ) {
                         LOG_SYS_ERR ( "Inconsistent queue !!! \n" );
                         continue;
                    }

                    LOG_SYS_LOG ( "State update received\n" );
                    // search if we must update a saved status value
                    linkedlist_node_t* node = linkedlist_searchNode ( &ssw_state.statusMessages, SystemStateWatcher_SearchStatusUpdate , &status );

                    // copy status informations
                    Status_Update_t* cp = malloc ( sizeof ( Status_Update_t ) );
                    memcpy ( cp, &status, sizeof ( Status_Update_t ) );

                    if ( node == NULL ) {
                         node = linkedlist_createNode ( cp );
                         linkedlist_pushToFront ( &ssw_state.statusMessages, node );
                    } else {
                         free ( node->data );
                         node->data = cp;
                    }
                    // propagate update
                    SystemStateWatcher_propagateUpdate ( &status );

                    // freeSemaphore
                    xSemaphoreGive ( ssw_state.exclusiveDataAccess );
               } else {
                    LOG_SYS_ERR ( "Failed to store status update, since lock couldn't be set\n" );
               }
          } else {
               // no updates for 1500 ms, see if we should cleanup old sockets
               if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) ( 20 / portTICK_RATE_MS ) ) == pdTRUE ) {
                    linkedlist_foreach ( &ssw_state.connections, SystemStateWatcher_CleanConnection, NULL );
                    linkedlist_cleanup ( &ssw_state.connections );
// 		    if(ssw_state.connections.elements == 0){
// 		      LOG_SYS_L("Connection list empty\n");
// 		    }
                    xSemaphoreGive ( ssw_state.exclusiveDataAccess );
               }
          }
     }
}


// Init dispatcher
void SystemStateWatcher_Task_init()
{
     ssw_state.systemStateQueue = xQueueCreate ( 15, sizeof ( Status_Update_t ) );
     if ( ssw_state.systemStateQueue == 0 ) {
          LOG_SYS_ERR ( "systemStateQueue creation failed\n" );
     }

     ssw_state.statusMessages = createLinkedList();
     ssw_state.connections = createLinkedList();

     vSemaphoreCreateBinary ( ssw_state.exclusiveDataAccess );

     xTaskCreate ( SystemStateWatcher_Task_thread, ( const signed char * const ) "SystemState_Task",
                   configMINIMAL_STACK_SIZE, NULL, SYSTEMSTATE_TASK_PRIO, NULL );
}


/**
 * Put Status_Update_t* into local queue, contained payload.raw is not copied and must not be used
 * futher. It will also be freed automatically.
 */
void SystemStateWatcher_Enqueue ( Status_Update_t* status )
{
     if ( xQueueSend ( ssw_state.systemStateQueue, status , 20 / portTICK_RATE_MS ) == errQUEUE_FULL ) {
          // free data if command can not stored
          LOG_SYS_LOG ( "Dispatcher: Queue full, dropping command\n" );
          free ( status->payload.raw );
     } else {
          LOG_SYS_LOG ( "Received status update from %d\n", ( int ) status->key.fromComponent );
     }
}

bool SystemStateWatcher_registerConnection ( ConnectionHandler_t* connection )
{
     if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) ( 1000 / portTICK_RATE_MS ) == pdTRUE ) ) {
          if ( xSemaphoreTake ( connection->connectionFreeSemaphore, ( portTickType ) ( 10 / portTICK_RATE_MS ) ) == pdFALSE ) {
               LOG_SYS_ERR ( "SystemStateWatcher_registerConnection connectionFreeSemaphore log failed\n" );
               return false;
          }
          // Implement implement full dump
          linkedlist_foreach ( &ssw_state.statusMessages, SystemStateWatcher_TransmitStatusUpdate2 ,connection );

          // add connection
          linkedlist_node_t* node = linkedlist_createNode ( connection );
          linkedlist_pushToFront ( &ssw_state.connections, node );

          xSemaphoreGive ( ssw_state.exclusiveDataAccess );
          return true;
     } else {
          LOG_SYS_ERR ( "SystemStateWatcher_registerConnection creation failed\n" );
     }
     return false;
}
