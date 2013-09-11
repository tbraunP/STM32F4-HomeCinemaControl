#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/IncomingDataHandler.h"

#include "lwip/opt.h"


#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

volatile int threads = 0;
#define MAXTHREADS	(2)

/*-----------------------------------------------------------------------------------*/
void IncomingDataHandler_thread ( void *arg )
{
     struct netconn *connection = ( struct netconn * ) arg;
     err_t xErr;

     struct netbuf *buf;
     void *data;
     u16_t len;

     while ( ( xErr = netconn_recv ( connection, &buf ) ) == ERR_OK ) {
          do {
               netbuf_data ( buf, &data, &len );
               netconn_write ( connection, data, len, NETCONN_COPY );

          } while ( netbuf_next ( buf ) >= 0 );

          netbuf_delete ( buf );
     }

     /* Close connection and discard connection identifier. */
     netconn_close ( connection );
     netconn_delete ( connection );
     vPortEnterCritical();
     --threads;
     vPortExitCritical();
}
/*-----------------------------------------------------------------------------------*/

bool NewIncomingDataHandlerTask ( void* connection )
{
     bool result = false;
     vPortEnterCritical();
     if ( threads < MAXTHREADS ) {
          xTaskCreate ( IncomingDataHandler_thread, ( const signed char * const ) "IncomingData",
                        configMINIMAL_STACK_SIZE, connection, TCPINCOMINGDATAHandler_TASK_PRIO, NULL );
          result =  true;
          ++threads;
     } else {
          /* Close connection and discard connection identifier. */
          struct netconn *connection = ( struct netconn * ) connection;
          netconn_close ( connection );
          netconn_delete ( connection );
     }
     vPortExitCritical();
     return result;
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
