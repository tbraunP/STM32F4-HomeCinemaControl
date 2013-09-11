#ifndef INCOMINGCONNECTIONLISTENER_H_
#define INCOMINGCONNECTIONLISTENER_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LISTENPORT		(666)  
  
/**
 * Listen on port for incoming connections
 */
void IncomingConnectionListener_thread ( void *arg );

/**
 * Create a new thread listening for incoming connections
 * 
 */ 
void IncomingConnectionListener_Task_init ( void );

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */