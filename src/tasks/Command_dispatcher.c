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

// Logging
#ifdef ENABLE_LOG_SS
  #define LOG_DISP_LOG( ...)	printf( __VA_ARGS__ )
  #define LOG_DISP_ERR( ...) 	printf( __VA_ARGS__ )
#else
  #define LOG_DISP_LOG( ...)
  #define LOG_DISP_ERR( ...)	printf( __VA_ARGS__)
#endif

// Queues to components to transfer commands of typ Command_t
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
               LOG_DISP_ERR ( "Dispatcher: Queue full, dropping command\n" );
               free ( command->payload.raw );
          }
     } else {
          LOG_DISP_ERR ( "Dispatcher: Unknown component\n" );
          free ( command->payload.raw );
     }
}


