#ifndef WS2803TASK_H_
#define WS2803TASK_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * Command to set specific state
 */
typedef struct WS2803Command_t{
  uint8_t led;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} WS2803Command_t;

/**
 * Commandqueue for ws2803
 */
extern xQueueHandle ws2803Queue;

/**
 * Init WS2803 Task
 */
void WS2803_Task_init();

void WS2803_thread(void *arg);


#ifdef __cplusplus
 }
#endif

#endif // SOLIDSTATETASK.H_H
