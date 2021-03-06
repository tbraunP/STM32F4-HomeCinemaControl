#ifndef INCOMINGCONNECTIONHANDLER_H_
#define INCOMINGCONNECTIONHANDLER_H_

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "util/ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

// PhysicalFrame Layout
typedef struct PhysicalFrame_t{
  size_t len;
  uint8_t* payload;
}PhysicalFrame_t;
  
// Handle to provide a connection for SystemStateWatcher
typedef struct ConnectionHandler_t {
     xQueueHandle connectionQueue;
     xSemaphoreHandle connectionFreeSemaphore;
     volatile bool connectionBroken;
} ConnectionHandler_t;


/**
 * Create a new thread to handle incoming frames
 * \param connection - incoming connection
 * \result bool if new thread has been spawned, otherwise the connection is rejected and closed
 */
bool NewConnectionHandlerTask ( void* connection );

#ifdef __cplusplus
}
#endif

#endif /* INCOMINGCONNECTIONHANDLER_H_ */
