#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "tasks/Task_Priorities.h"
#include "tasks/IncomingDataHandler.h"
#include "tasks/command_dispatcher.h"
#include "tasks/systemStateWatcher.h"

#include "lwip/opt.h"


#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

volatile int threads = 0;
#define MAXTHREADS	(2)

typedef enum ParseState_t {
     init, preambleFound, headerFound
}
ParseState_t;


typedef struct IncomingDataHandler_t {
     IncomingConnection_t con;
     struct netconn *netconnection;

     ringbuf_t incomingRingBuffer;
     ParseState_t state;

     uint8_t length, type, component;
     uint16_t received;
} IncomingDataHandler_t;


void IncomingDataHandler_frameFound ( IncomingDataHandler_t* threadState, uint8_t type, uint8_t component, uint8_t* payload, size_t len )
{
     if ( type==0x01 ) {
          ++ ( threadState->received );
          Command_t command = { .len = len, .component = component, .payload.raw = payload};
          Dispatcher_dispatch ( &command );
//        printf ( "Frame received\n" );
//        printf ( "Frame of type %d for component %d received\n", ( int ) type, ( int ) component );
     } else {
//        printf ( "Unknown frametype\n" );
          free ( payload );
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
                    uint8_t* payload = NULL;
                    if ( len != 0 ) {
                         payload = malloc ( len );
                         rb_read ( dataBuffer, payload, len );
                    }
                    // frameFound cares also about data freeing  or forwards to components caring about the freeing
                    IncomingDataHandler_frameFound ( threadState, threadState->type, threadState->component, payload, len );

                    // remove trailer from buffer
                    for ( int i = 0; i < 2; i++ )
                         rb_getc ( dataBuffer, &tmp );

                    // output buffer state
                    //printf("DatabufferLen %d\n", (int) dataBuffer->len);
               } else {
                    // something went wrong, retry
                    rb_getc ( dataBuffer, &tmp );
               }
               threadState->state = init;
               break;
          }
          default:
               printf ( "WTF unknown state\n" );
          }
     }
}

/*-----------------------------------------------------------------------------------*/
/**
 * Handle incoming frames
 */
void IncomingDataHandler_thread ( void *arg )
{
     IncomingDataHandler_t* lArg = ( IncomingDataHandler_t* ) arg;
     struct netconn *netconnection = lArg->netconnection;

     err_t xErr;

     struct netbuf *buf;
     void *data;
     u16_t len;

     // register connection a systemWatcher
     SystemStateWatcher_registerConnection ( &lArg->con );

     while ( ( xErr = netconn_recv ( netconnection, &buf ) ) == ERR_OK ) {
          do {
               netbuf_data ( buf, &data, &len );
               //netconn_write ( netconnection, data, len, NETCONN_COPY );
               rb_write ( & ( lArg->incomingRingBuffer ), ( const uint8_t * ) data, len );
          } while ( netbuf_next ( buf ) >= 0 );

          netbuf_delete ( buf );

          // try to parse frame
          //printf("Parsing...\n");
          IncomingDataHandler_parseFrame ( lArg );
          //printf("Waiting for new data\n");
     }

     /* Close connection and discard connection identifier. */
     // wait until listener has given up the semaphore
     lArg->con.connectionBroken = true;
     while ( xSemaphoreTake ( lArg->con.connectionFreeSemaphore, ( portTickType ) portMAX_DELAY ) != pdTRUE );

     // clos connection and cleanup
     netconn_close ( netconnection );
     netconn_delete ( netconnection );
     rb_free ( & ( lArg->incomingRingBuffer ) );

     // free handle for systemStateWatcher
     vQueueDelete ( lArg->con.connection );
     vSemaphoreDelete ( lArg->con.connectionFreeSemaphore );

     // free state container
     free ( lArg );

     vPortEnterCritical();
     --threads;
     vPortExitCritical();
     printf ( "Terminating IncomingDataHandler %d\n", ( int ) lArg->received );

     vTaskDelete ( NULL );
}
/*-----------------------------------------------------------------------------------*/

bool NewIncomingDataHandlerTask ( void* connection )
{
     bool result = false;
     vPortEnterCritical();
     if ( threads < MAXTHREADS ) {
          IncomingDataHandler_t* threadState = malloc ( sizeof ( IncomingDataHandler_t ) );

          // connection related stuff for SystemStateWatcher
          threadState->con.connection = xQueueCreate ( 30, sizeof ( PhysicalFrame_t ) );;
          threadState->con.connectionBroken = false;
          vSemaphoreCreateBinary ( threadState->con.connectionFreeSemaphore );

          // internal state
          threadState->netconnection = ( struct netconn * ) connection;
          threadState->state = init;
          threadState->received = 0;
          rb_alloc ( & ( threadState->incomingRingBuffer ), 800 );

	  // create thread
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
