#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "tasks/systemStateWatcher.h"

#include "irmp/irmp.h"
#include "tasks/solidStateTask.h"
#include "tasks/irmpTask.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tasks/Task_Priorities.h"
#include "util/linkedlist.h"

struct {
     linkedlist_t statusMessages;
     linkedlist_t connections;
     // exclusive access to statusMessage and connection container
     xSemaphoreHandle exclusiveDataAccess;

     xQueueHandle systemStateQueue;
} ssw_state;



void SystemStateWatcher_CleanConnection ( linkedlist_node_t* node )
{
     IncomingConnection_t* connection = ( IncomingConnection_t* ) node->data;
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

/**
 * Propagate a status update
 * \param status - Status update to propagate
 */
void SystemStateWatcher_propagateUpdate ( Status_Update_t* status )
{
}

/* Thread to Handle status updates */
void SystemStateWatcher_Task_thread()
{
     static Status_Update_t status;

     for ( ;; ) {
          if ( xQueueReceive ( ssw_state.systemStateQueue, & ( status ), ( portTickType ) ( 1500/portTICK_RATE_MS ) ) ) {
               if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) portMAX_DELAY ) == pdTRUE ) {
                    // search if we must update a saved status value
                    linkedlist_node_t* node = linkedlist_searchNode ( &ssw_state.statusMessages, SystemStateWatcher_SearchStatusUpdate , &status );

                    // copy status informations
                    Status_Update_t* cp = malloc ( sizeof ( Status_Update_t ) );
                    memcpy ( cp, &status, sizeof ( Status_Update_t ) );

                    if ( node == NULL ) {
                         node = linkedlist_createNode ( cp );
                         linkedlist_pushToFront ( &ssw_state.statusMessages, node );
                    } else {
                    }

                    SystemStateWatcher_propagateUpdate ( &status );

                    // freeSemaphore
                    xSemaphoreGive ( ssw_state.exclusiveDataAccess );
               }
          } else {
               // no updates for 1500 ms, see if we should cleanup old sockets
               if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) 10 ) == pdTRUE ) {
                    linkedlist_processList ( &ssw_state.connections, SystemStateWatcher_CleanConnection );
                    linkedlist_cleanup ( &ssw_state.connections );
// 		    if(ssw_state.connections.elements == 0){
// 		      printf("Connection list empty\n");
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
          printf ( "systemStateQueue creation failed\n" );
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
          printf ( "Dispatcher: Queue full, dropping command\n" );
          free ( status->payload.raw );
     }
}

void SystemStateWatcher_registerConnection ( IncomingConnection_t* connection )
{
     if ( xSemaphoreTake ( ssw_state.exclusiveDataAccess, ( portTickType ) portMAX_DELAY ) == pdTRUE ) {
          // TODO Implement implement full dump

          // add connection
          linkedlist_node_t* node = linkedlist_createNode ( connection );
          linkedlist_pushToFront ( &ssw_state.connections, node );

          xSemaphoreGive ( ssw_state.exclusiveDataAccess );
     }
}
