/*
 *  Created on: 13.09.2013
 *      Author: pyro
 */

#ifndef SYSTEMSTATEWATCHER_H_
#define SYSTEMSTATEWATCHER_H_
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// includes for status messages payload
#include "irmp/irmp.h"
#include "tasks/SolidStateTask.h"
#include "tasks/IrmpTask.h"
#include "tasks/IncomingConnectionHandler.h"

// lwIP
#include "lwip/tcpip.h"

#ifdef __cplusplus
extern "C" {
#endif

// add new status update
// fromComponent and uuid identify are memory slot, where the status message is stored resp. updated
// if a newer message with these keys appear
typedef struct __attribute__ ( ( __packed__ ) ) Status_Update_t {
     struct {
      uint8_t fromComponent;
      // uuid to identifie the status to be updated
      uint8_t uuid;
     } key;
     
     // length of the raw encoded payload
     size_t len;
     
     // payload must be stored on the heap!
union __attribute__ ( ( __packed__ ) ) {
     IRMP_Status_t* irmpStatus;
     SolidStateStatus_t* solidStateStatus;
     uint8_t* raw;
} payload;
} Status_Update_t;


void SystemStateWatcher_Task_init();

/**
 * Put Status_Update_t* into local queue, contained payload.raw is not copied and must not be used
 * futher. It will also be freed automatically.
 */ 
void SystemStateWatcher_Enqueue ( Status_Update_t* status );

/**
 * Register a new connection
 */
bool SystemStateWatcher_registerConnection( ConnectionHandler_t* connection );

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */
