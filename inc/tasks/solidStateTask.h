#ifndef SOLIDSTATETASK_H_
#define SOLIDSTATETASK_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"

typedef enum {
  BEAMER = 0,
  LEINWAND = 1,
  AMPLIFIER = 2
} SolidStateRelais_t;

typedef enum {
  ON, OFF
} SolidStateRelais_Mode_t; 

/**
 * Command to set specific state
 */
typedef struct SolidStateCommand_t{
  SolidStateRelais_t relais;
  SolidStateRelais_Mode_t newState;
}SolidStateCommand_t;

/**
 * Commandqueue for solidStateTask
 */
extern xQueueHandle solidStateQueue;

/**
 * Init solid state task and activate task
 */
void SolidState_Task_init();

void SolidState_thread(void *arg);

/**
 * Read actual state
 */
SolidStateRelais_Mode_t SolidState_getState(SolidStateRelais_t relais);

#endif // SOLIDSTATETASK.H_H
