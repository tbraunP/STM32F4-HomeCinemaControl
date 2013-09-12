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

#include "tasks/command_dispatcher.h"

/*-----------------------------------------------------------------------------------*/
xQueueHandle irsndQueue;

void IRSND_thread ( void *arg )
{
     static Command_t incoming;
     static IRMP_DATA* irmp_data;

     for ( ;; ) {
          if ( xQueueReceive ( solidStateQueue, & ( incoming ), ( portTickType ) portMAX_DELAY ) ) {
               irmp_data = incoming.irsndData;

               printf ( "IRSND starting to transmit\n" );
               //irmp_data->protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
               //irmp_data.address = 0x00FF; // set address to 0x00FF
               //irmp_data.command = 0x0001; // set command to 0x0001
               //irmp_data.flags = 0; // don't repeat frame

               printf ( "Sending\nprotocol %x \n", ( ( int ) irmp_data->protocol ) &0xFF );
               printf ( "address %x \n", ( ( int ) irmp_data->address ) &0xFFFF );
               printf ( "command %x \n", ( ( int ) irmp_data->command ) &0xFFFF );
               printf ( "flags %x \n", ( ( int ) irmp_data->flags ) &0xFF );

               irsnd_send_data ( irmp_data, TRUE ); // send frame, wait for completion

               free ( incoming.raw );
               vTaskDelay ( 100/ portTICK_RATE_MS );
          }

     }
}

/*-----------------------------------------------------------------------------------*/
xQueueHandle irmpQueue;

void IRMP_thread ( void *arg )
{
     static Command_t incoming;
     static IRMP_Command_t* irmpCommand;
     static IRMP_DATA irmp_data;

     portTickType delay = portMAX_DELAY;

     for ( ;; ) {
          //

          if ( xQueueReceive ( solidStateQueue, & ( incoming ), delay ) ) {
               // IRMP command
               irmpCommand = incoming.irmpCommand;

               if ( irmpCommand->mode == IRMP_ON ) {
                    printf ( "IRMP receiver activated\n" );
                    delay = 750 / portTICK_RATE_MS;

                    if ( irmp_get_data ( &irmp_data ) ) {
                         printf ( "Receiving\nprotocol %x \n", ( ( int ) irmp_data.protocol ) &0xFF );
                         printf ( "address %x \n", ( ( int ) irmp_data.address ) &0xFFFF );
                         printf ( "command %x \n", ( ( int ) irmp_data.command ) &0xFFFF );
                         printf ( "flags %x \n", ( ( int ) irmp_data.flags ) &0xFF );

                         // set to zero
                         memset ( &irmp_data, 0, sizeof ( IRMP_DATA ) );

                         // TODO: transmit frame to handy

                    }
               } else {
                    // wait forever, until new command arrives
                    printf ( "IRMP receiver deactivated\n" );
                    delay = portMAX_DELAY;
               }
               // free command
               free ( incoming.raw );
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
     irsndQueue = xQueueCreate ( 4, sizeof ( Command_t ) );


     xTaskCreate ( IRSND_thread, ( const signed char * const ) "IRSND_Task",
                   configMINIMAL_STACK_SIZE, NULL, IRSND_TASK_PRIO, NULL );

     xTaskCreate ( IRMP_thread, ( const signed char * const ) "IRMP_Task",
                   configMINIMAL_STACK_SIZE, NULL, LED_TASK_PRIO, NULL );
}
/*-----------------------------------------------------------------------------------*/

