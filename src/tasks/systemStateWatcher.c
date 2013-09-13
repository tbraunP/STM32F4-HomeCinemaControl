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
#include "tasks/IncomingDataHandler.h"

struct {
     linkedlist_t statusMessages;
     linkedlist_t connections;

     xSemaphoreHandle connectionSemaphore;

     xQueueHandle systemStateQueue;
} ssw_state;



void SystemStateWatcher_CleanConnection ( void* incomingDataHandler )
{
     IncomingDataHandler_t* connection = ( IncomingDataHandler_t* ) incomingDataHandler;

}

void SystemStateWatcher_Task_thread()
{
     Status_Update_t status;

     for ( ;; ) {
          if ( xQueueReceive ( ssw_state.systemStateQueue, & ( status ), ( portTickType ) ( 1500/portTICK_RATE_MS ) ) ) {

          } else {
               // no updates for 1500 ms, see if we should clean up old sockets
               if ( xSemaphoreTake ( ssw_state.connectionSemaphore, ( portTickType ) 10 ) == pdTRUE ) {
                    linkedlist_processList ( &ssw_state.connections, SystemStateWatcher_CleanConnection );
                    xSemaphoreGive ( ssw_state.connectionSemaphore );
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

     vSemaphoreCreateBinary ( ssw_state.connectionSemaphore );

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
