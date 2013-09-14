#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks/Task_Priorities.h"
#include "tasks/irmpTask.h"

#include "irmp/irsnd.h"
#include "irmp/irmp.h"
#include "irmp/irmp_stm32f4.h"

// dispatcher
#include "tasks/command_dispatcher.h"
#include "tasks/systemStateWatcher.h"

/*-----------------------------------------------------------------------------------*/
xQueueHandle irsndQueue;

void IRSND_thread ( void *arg )
{
     static Command_t incoming;
     static IRMP_DATA* irmp_data;

     for ( ;; ) {
          if ( xQueueReceive ( irsndQueue, & ( incoming ), ( portTickType ) portMAX_DELAY ) ) {
               irmp_data = incoming.payload.irsndData;

               printf ( "IRSND starting to transmit\n" );
               //irmp_data->protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
               //irmp_data.address = 0x00FF; // set address to 0x00FF
               //irmp_data.command = 0x0001; // set command to 0x0001
               //irmp_data.flags = 0; // don't repeat frame

#if 0
               printf ( "Size %d (IRMPData) and payload len %d\n", ( int ) sizeof ( IRMP_DATA ), ( int ) incoming.len );
               printf ( "Sending\nprotocol %x \n", ( ( int ) irmp_data->protocol ) &0xFF );
               printf ( "Sending\nprotocol %x \n", ( ( int ) irmp_data->protocol ) &0xFF );
               printf ( "address %x \n", ( ( int ) irmp_data->address ) &0xFFFF );
               printf ( "command %x \n", ( ( int ) irmp_data->command ) &0xFFFF );
               printf ( "flags %x \n", ( ( int ) irmp_data->flags ) &0xFF );
#endif
               irsnd_send_data ( irmp_data, TRUE ); // send frame, wait for completion

               free ( incoming.payload.raw );
               vTaskDelay ( ( portTickType ) 100 );
          }
     }
}

/*-----------------------------------------------------------------------------------*/
xQueueHandle irmpQueue;

static inline void IRMP_sendIRMPData ( IRMP_DATA* irmp_data )
{
     // transmit frame to status update
     IRMP_Status_t* ldt = malloc ( sizeof ( IRMP_Status_t ) );
     ldt->messageType = 0x01;
     memcpy ( & ( ldt->payload.irmpData ), irmp_data, sizeof ( IRMP_DATA ) );

     Status_Update_t status = { .key.fromComponent= IRMP, .key.uuid = 0x01, .len = sizeof ( IRMP_DATA ), .payload.irmpStatus = ldt };
     SystemStateWatcher_Enqueue ( &status );
}

static void IRMP_sendOnOffState ( IRMP_Command_Mode_t mode )
{
     // transmit frame to status update
     IRMP_Status_t* ldt = malloc ( sizeof ( IRMP_Status_t ) );
     ldt->messageType = 0x00;
     ldt->payload.irmpCommand = mode;

     Status_Update_t status = { .key.fromComponent= IRMP, .key.uuid = 0x00, .len= sizeof ( IRMP_Command_Mode_t ), .payload.irmpStatus = ldt };
     SystemStateWatcher_Enqueue ( &status );
}


void IRMP_readIncomingFrame()
{
     // set to zero
     static IRMP_DATA irmp_data;
     memset ( &irmp_data, 0, sizeof ( IRMP_DATA ) );

     if ( irmp_get_data ( &irmp_data ) ) {
#if 0
          printf ( "Receiving\nprotocol %x \n", ( ( int ) irmp_data.protocol ) &0xFF );
          printf ( "address %x \n", ( ( int ) irmp_data.address ) &0xFFFF );
          printf ( "command %x \n", ( ( int ) irmp_data.command ) &0xFFFF );
          printf ( "flags %x \n", ( ( int ) irmp_data.flags ) &0xFF );
#endif
          // transmit frame to status update
          IRMP_sendIRMPData ( &irmp_data );
     }

}

void IRMP_thread ( void *arg )
{
     static Command_t incoming;
     static IRMP_Command_t* irmpCommand;

     static IRMP_Command_Mode_t mode = IRMP_OFF;
     IRMP_sendOnOffState ( mode );

     portTickType delay = portMAX_DELAY;

     for ( ;; ) {
          if ( xQueueReceive ( irmpQueue, & ( incoming ), delay ) ) {
               // IRMP command
               irmpCommand = incoming.payload.irmpCommand;
               IRMP_sendOnOffState ( irmpCommand->mode );

               if ( irmpCommand->mode == IRMP_ON ) {
                    printf ( "IRMP receiver activated\n" );
                    delay = 750 / portTICK_RATE_MS;
                    IRMP_readIncomingFrame();
                    mode = IRMP_ON;
               } else {
                    // wait forever, until new command arrives
                    //printf ( "IRMP receiver deactivated\n" );
                    delay = portMAX_DELAY;
               }
               // free command
               free ( incoming.payload.raw );
          } else {
               // delay caused execution, since no frame has been received
               if ( mode == IRMP_ON ) {
                    IRMP_readIncomingFrame();
               }
          }
     }
}


/*-----------------------------------------------------------------------------------*/

void IRSND_Task_init()
{
     /* Initialize IRMP */
     IRMP_Init();

     // create queues
     irmpQueue = xQueueCreate ( 2, sizeof ( Command_t ) );
     if ( irmpQueue == 0 ) {
          printf ( "irmpQueue queue creation failed\n" );
     }

     irsndQueue = xQueueCreate ( 4, sizeof ( Command_t ) );
     if ( irsndQueue == 0 ) {
          printf ( "irsndQueue queue creation failed\n" );
     }

     xTaskCreate ( IRSND_thread, ( const signed char * const ) "IRSND_Task",
                   configMINIMAL_STACK_SIZE, NULL, IRSND_TASK_PRIO, NULL );

     xTaskCreate ( IRMP_thread, ( const signed char * const ) "IRMP_Task",
                   configMINIMAL_STACK_SIZE, NULL, LED_TASK_PRIO, NULL );
}
/*-----------------------------------------------------------------------------------*/

