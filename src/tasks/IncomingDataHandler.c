#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/IncomingDataHandler.h"

#include "util/ringbuf.h"

#include "lwip/opt.h"


#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

volatile int threads = 0;
#define MAXTHREADS	(2)

typedef struct IncomingDataHandler_t{
  struct netconn *connection;
  ringbuf_t incomingRingBuffer;
}IncomingDataHandler_t;

void IncomingDataHandler_parseFrame(IncomingDataHandler_t* threadState){
}

/*-----------------------------------------------------------------------------------*/
void IncomingDataHandler_thread ( void *arg )
{
     IncomingDataHandler_t* lArg = (IncomingDataHandler_t*) arg;
     struct netconn *connection = lArg->connection;
     
     err_t xErr;

     struct netbuf *buf;
     void *data;
     u16_t len;

     while ( ( xErr = netconn_recv ( connection, &buf ) ) == ERR_OK ) {
          do {
               netbuf_data ( buf, &data, &len );
               netconn_write ( connection, data, len, NETCONN_COPY );
	       rb_write(&lArg->incomingRingBuffer, (const uint8_t *) data, len);
          } while ( netbuf_next ( buf ) >= 0 );

          netbuf_delete ( buf );
	  
	  // try to parse frame
	  IncomingDataHandler_parseFrame(lArg);
     }

     /* Close connection and discard connection identifier. */
     netconn_close ( connection );
     netconn_delete ( connection );
     rb_free(&(lArg->incomingRingBuffer));
     free(lArg);
     
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
	  IncomingDataHandler_t* threadState = malloc(sizeof(IncomingDataHandler_t));
	  threadState->connection = connection;
	  rb_alloc(&(threadState->incomingRingBuffer), 512);
       
          xTaskCreate ( IncomingDataHandler_thread, ( const signed char * const ) "IncomingData",
                        configMINIMAL_STACK_SIZE, threadState, TCPINCOMINGDATAHandler_TASK_PRIO, NULL );
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
