#ifndef INCOMINGDATAHANDLER_H_
#define INCOMINGDATAHANDLER_H_

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
typedef struct IncomingConnection_t {
     xQueueHandle connection;
     xSemaphoreHandle connectionFreeSemaphore;
     volatile bool connectionBroken;
} IncomingConnection_t;


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
