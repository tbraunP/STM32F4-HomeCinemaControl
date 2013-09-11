#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/IncomingConnectionListener.h"
#include "tasks/IncomingDataHandler.h"

#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"


/*-----------------------------------------------------------------------------------*/
void IncomingConnectionListener_thread ( void *arg )
{
     struct netconn *conn, *newconn;
     err_t err;

     LWIP_UNUSED_ARG ( arg );

     /* Create a new connection identifier. */
     conn = netconn_new ( NETCONN_TCP );

     if ( conn!=NULL ) {
          /* Bind connection to well known port number 7. */
          err = netconn_bind ( conn, NULL, 7 );

          if ( err == ERR_OK ) {
               /* Tell connection to go into listening mode. */
               netconn_listen ( conn );

               while ( 1 ) {
                    /* Grab new connection. */
                    err_t xErr = netconn_accept ( conn, &newconn );

                    /* Process the new connection. */
                    if ( xErr== ERR_OK ) {
			 printf("Spawning new process for incoming commands\n");
                         NewIncomingDataHandlerTask ( newconn );
                    }
               }
          } else {
               printf ( " can not bind TCP netconn" );
          }
     } else {
          printf ( "can not create TCP netconn" );
     }
}
/*-----------------------------------------------------------------------------------*/

void IncomingConnectionListener_Task_init ( void )
{
     xTaskCreate ( IncomingConnectionListener_thread, ( const signed char * const ) "ConnectionListener",
                   configMINIMAL_STACK_SIZE, NULL, TCPINCOMINGListener_TASK_PRIO, NULL );
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
