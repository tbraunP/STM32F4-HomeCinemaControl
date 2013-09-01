#ifndef IRMPTASK_H_
#define IRMPTASK_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void IRSND_thread(void *arg);

void IRSND_Task_init();

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */