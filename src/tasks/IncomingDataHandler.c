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

typedef enum ParseState_t {
     init, preambleFound, headerFound
} ParseState_t;

typedef struct IncomingDataHandler_t {
     struct netconn *connection;
     ringbuf_t incomingRingBuffer;
     ParseState_t state;

     uint8_t length, type, component;
} IncomingDataHandler_t;

void IncomingDataHandler_frameFound ( uint8_t type, uint8_t component, uint8_t* payload, size_t len )
{
  if(type==0x01){
    printf("Frame of type %d for component %d received\n", (int) type, (int) component);
  }else{ 
    printf("Unknown frametype\n");
  }
}

void IncomingDataHandler_parseFrame ( IncomingDataHandler_t* threadState )
{
     uint8_t tmp;
     ringbuf_t* dataBuffer = & ( threadState->incomingRingBuffer );
     for ( ;; ) {
          switch ( threadState->state ) {
          case init: {
               // search preamble
               while ( dataBuffer->len >= 2 ) {
                    if ( rb_peek ( dataBuffer, 0 ) == ( 0xAB )
                              && rb_peek ( dataBuffer, 1 ) == ( 0xCD ) ) {
                         threadState->state = preambleFound;
                         break;
                    } else {
                         // remove one byte and try again to match preamble
                         rb_getc ( dataBuffer, &tmp );
                    }
               }
               // wait for more data
               if ( threadState->state != preambleFound )
                    return;
          }
          case preambleFound: {
               // wait for more data
               if ( dataBuffer->len < ( 4 + 2 ) )
                    return;
               // interpret header
               threadState->length = rb_peek ( dataBuffer, 2 );
               threadState->type = rb_peek ( dataBuffer, 3 );
               threadState->component = rb_peek ( dataBuffer,4 );

               // check valid header format
               if ( rb_peek ( dataBuffer, 5 ) == ( 0xFF ) ) {
                    threadState->state = headerFound;
               } else {
                    // preamble seems to be not valid, so try again with
                    // next one
                    rb_getc ( dataBuffer, &tmp );
                    threadState->state = init;
                    break;
               }
          }
          case headerFound: {
               // wait for more data
               if ( dataBuffer->len < threadState->length )
                    return;

               // look for end
               if ( rb_peek ( dataBuffer,threadState->length - 2 ) == ( 0xEF )
                         && rb_peek ( dataBuffer, threadState->length - 1 ) == ( 0xFE ) ) {

                    // remove prefix and header
                    for ( int i=0; i< 6; i++ )
                         rb_getc ( dataBuffer, &tmp );

                    // copy payload and construct frame
                    size_t len = threadState->length - 8;
                    uint8_t* payload = malloc ( len );
                    rb_read ( dataBuffer, payload, len );
                    IncomingDataHandler_frameFound ( threadState->type, threadState->component, payload, len );

                    // remove trailer from buffer
                    for ( int i = 0; i < 2; i++ )
                         rb_getc ( dataBuffer, &tmp );
               } else {
                    // something went wrong, retry
                    rb_getc ( dataBuffer, &tmp );
                    threadState->state = init;
                    break;
               }
          }
          default:
               printf ( "WTF unknown state\n" );
          }
     }
}

/*-----------------------------------------------------------------------------------*/
void IncomingDataHandler_thread ( void *arg )
{
     IncomingDataHandler_t* lArg = ( IncomingDataHandler_t* ) arg;
     struct netconn *connection = lArg->connection;

     err_t xErr;

     struct netbuf *buf;
     void *data;
     u16_t len;

     while ( ( xErr = netconn_recv ( connection, &buf ) ) == ERR_OK ) {
          do {
               netbuf_data ( buf, &data, &len );
               //netconn_write ( connection, data, len, NETCONN_COPY );
               rb_write ( & ( lArg->incomingRingBuffer ), ( const uint8_t * ) data, len );
          } while ( netbuf_next ( buf ) >= 0 );

          netbuf_delete ( buf );

          // try to parse frame
          IncomingDataHandler_parseFrame ( lArg );
     }

     /* Close connection and discard connection identifier. */
     netconn_close ( connection );
     netconn_delete ( connection );
     rb_free ( & ( lArg->incomingRingBuffer ) );
     free ( lArg );

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
          IncomingDataHandler_t* threadState = malloc ( sizeof ( IncomingDataHandler_t ) );
          threadState->connection = connection;
          rb_alloc ( & ( threadState->incomingRingBuffer ), 512 );

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
