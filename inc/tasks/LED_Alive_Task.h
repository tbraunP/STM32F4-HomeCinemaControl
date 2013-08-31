/*
 * LED_Alive_Task.h
 *
 *  Created on: 31.08.2013
 *      Author: pyro
 */

#ifndef LED_ALIVE_TASK_H_
#define LED_ALIVE_TASK_H_
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

// lwIP
#include "lwip/tcpip.h"

#ifdef __cplusplus
extern "C" {
#endif

void LED_ToggleLed_ALIVE(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */
