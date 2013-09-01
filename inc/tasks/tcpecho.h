#ifndef TCPECHO_H_
#define TCPECHO_H_

#ifdef __cplusplus
extern "C" {
#endif

void tcpecho_thread(void *arg);

void tcpecho_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_ALIVE_TASK_H_ */