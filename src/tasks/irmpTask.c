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
     bool hasSend = false;

     for ( ;; ) {
          printf ( "IRSND starting to transmit\n" );
          irmp_data.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
          irmp_data.address = 0x00FF; // set address to 0x00FF
          irmp_data.command = 0x0001; // set command to 0x0001
          irmp_data.flags = 0; // don't repeat frame
          if(!hasSend){
	    irsnd_send_data ( &irmp_data, TRUE ); // send frame, wait for completion
	    hasSend = true;
	  }

          // wait one second till next transmission
          vTaskDelay ( 5000 );
     }
}

void IRMP_thread ( void *arg )
{
     static IRMP_DATA irmp_data;

     for ( ;; ) {
          printf ( "IRMP receiving\n" );

          if ( irmp_get_data ( &irmp_data ) ) {
               printf ( "Receiving\nprotocol %x \n", ( ( int ) irmp_data.protocol ) &0xFF );
	       printf ( "address %x \n", ( ( int ) irmp_data.address ) &0xFFFF );
	       printf ( "command %x \n", ( ( int ) irmp_data.command ) &0xFFFF );
	       printf ( "flags %x \n", ( ( int ) irmp_data.flags ) &0xFF);
                    
	       // set to zero
	       memset(&irmp_data, 0, sizeof(IRMP_DATA));
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

