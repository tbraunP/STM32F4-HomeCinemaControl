#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "tasks/Command_dispatcher.h"

#include "irmp/irmp.h"
#include "tasks/SolidStateTask.h"
#include "tasks/IrmpTask.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static xQueueHandle* COMPONENT_QUEUES[MAX_COMPONENTS];

// Init dispatcher
void Dispatcher_init()
{
     COMPONENT_QUEUES[IRSND] = &irsndQueue;
     COMPONENT_QUEUES[IRMP] = &irmpQueue;
     COMPONENT_QUEUES[SOLIDSTATE] = &solidStateQueue;
}

// forward command to
void Dispatcher_dispatch ( Command_t* command )
{
     if ( command->component < MAX_COMPONENTS ) {
          if ( xQueueSend ( *COMPONENT_QUEUES[command->component], command , 20 / portTICK_RATE_MS ) == errQUEUE_FULL ) {
               // free data if command can not stored
               printf ( "Dispatcher: Queue full, dropping command\n" );
               free ( command->payload.raw );
          }
     } else {
          printf ( "Dispatcher: Unknown component\n" );
          free ( command->payload.raw );
     }
}


