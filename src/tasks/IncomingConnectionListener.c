#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/IncomingConnectionListener.h"
#include "tasks/IncomingConnectionHandler.h"
#include "tasks/Command_dispatcher.h"

#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"


// Logging
#ifdef ENABLE_LOG_ICL
  #define LOG_ICL_LOG( ...)	printf( __VA_ARGS__ )
  #define LOG_ICL_ERR( ...) 	printf( __VA_ARGS__ )
#else
  #define LOG_ICL_LOG( ...)
  #define LOG_ICL_ERR( ...)	printf( __VA_ARGS__)
#endif


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
          err = netconn_bind ( conn, NULL, LISTENPORT );

          if ( err == ERR_OK ) {
               /* Tell connection to go into listening mode. */
               netconn_listen ( conn );

               while ( 1 ) {
                    /* Grab new connection. */
                    err_t xErr = netconn_accept ( conn, &newconn );

                    /* Process the new connection. */
                    if ( xErr== ERR_OK ) {
                         LOG_ICL_ERR ( "Trying to spawn new process for incoming commands..." );
                         if ( NewConnectionHandlerTask ( newconn ) ) {
                              LOG_ICL_ERR ( "... successfull\n" );
                         } else {
                              LOG_ICL_LOG ( "... failed to many connections\n" );
                         }
                    }
               }
          } else {
               LOG_ICL_ERR ( " can not bind TCP netconn" );
          }
     } else {
          LOG_ICL_ERR ( "can not create TCP netconn" );
     }
     vTaskDelete ( NULL );
}
/*-----------------------------------------------------------------------------------*/

void IncomingConnectionListener_Task_init ( void )
{
     // initialize dispatcher
     Dispatcher_init();

     // listen to port for incoming command frames
     xTaskCreate ( IncomingConnectionListener_thread, ( const signed char * const ) "ConnectionListener",
                   configMINIMAL_STACK_SIZE, NULL, TCPINCOMINGListener_TASK_PRIO, NULL );
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
