#ifndef TCPECHO_H_
#define TCPECHO_H_
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"


#ifdef __cplusplus
extern "C" {
#endif

void tcpecho_thread(void *arg);

void tcpecho_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */