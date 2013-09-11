#ifndef INCOMINGDATAHANDLER_H_
#define INCOMINGDATAHANDLER_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle incoming frames
 */
void IncomingDataHandler_thread ( void *arg );

/**
 * Create a new thread to handle incoming frames
 * \param connection - incoming connection
 * \result bool if new thread has been spawned, otherwise the connection is rejected and closed
 */ 
bool NewIncomingDataHandlerTask ( void* connection );

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */