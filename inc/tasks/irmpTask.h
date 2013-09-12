#ifndef IRMPTASK_H_
#define IRMPTASK_H_
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif
  
typedef enum {
  IRMP_ON = 0x01, IRMP_OFF = 0x00
} IRMP_Command_Mode_t; 

// activate or deactivate receiver
typedef struct __attribute__((__packed__)) IRMP_Command_t{
  IRMP_Command_Mode_t mode;
}IRMP_Command_t;

// Definition of queues
extern xQueueHandle irmpQueue;
extern xQueueHandle irsndQueue;

// Start irmp and irsnd task
void IRSND_Task_init();

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */