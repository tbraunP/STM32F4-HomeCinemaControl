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

#include "tasks/Task_Priorities.h"
#include "util/linkedlist.h"

struct {
     linkedlist_t statusMessages;
     xQueueHandle systemStateQueue;
} ssw_state;



void SystemStateWatcher_Task_thread(){
  for(;;){
    
    
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
