#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/irmpTask.h"

#include "irmp/irsnd.h"
#include "irmp/irmp.h"
#include "irmp/irmp_stm32f4.h"


/*-----------------------------------------------------------------------------------*/
void IRSND_thread ( void *arg )
{
     static IRMP_DATA irmp_data;

     for ( ;; ) {
          printf ( "IRSND starting to transmit\n" );
          irmp_data.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
          irmp_data.address = 0x00FF; // set address to 0x00FF
          irmp_data.command = 0x0001; // set command to 0x0001
          irmp_data.flags = 0; // don't repeat frame
          irsnd_send_data ( &irmp_data, TRUE ); // send frame, wait for completion

          // wait one second till next transmission
          vTaskDelay ( 1000 );
     }
}

void IRMP_thread ( void *arg )
{
     static IRMP_DATA irmp_data;

     for ( ;; ) {
          printf ( "IRMP receiving\n" );

          if ( irmp_get_data ( &irmp_data ) ) {
               printf ( "Receiving %i \n", ( ( int ) irmp_data.protocol ) &0xFF );
               // ir signal decoded, do something here...
               // irmp_data.protocol is the protocol, see irmp.h
               // irmp_data.address is the address/manufacturer code of ir sender
               // irmp_data.command is the command code
               // irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
          }

          // wait one second till next transmission
          vTaskDelay ( 1000 );
     }
}


/*-----------------------------------------------------------------------------------*/

void IRSND_Task_init()
{
     /* Initialize IRMP */
     IRMP_Init();

     xTaskCreate ( IRSND_thread, ( const signed char * const ) "IRSND_Task",
                   configMINIMAL_STACK_SIZE, NULL, IRSND_TASK_PRIO, NULL );

     xTaskCreate ( IRMP_thread, ( const signed char * const ) "IRMP_Task",
                   configMINIMAL_STACK_SIZE, NULL, LED_TASK_PRIO, NULL );
}
/*-----------------------------------------------------------------------------------*/

