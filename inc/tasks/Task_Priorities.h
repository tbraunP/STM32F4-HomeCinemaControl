/*
 * Task_Priorities.h
 *
 *  Created on: 31.08.2013
 *      Author: pyro
 */
#ifndef TASK_PRIORITIES_H_
#define TASK_PRIORITIES_H_

#include "FreeRTOS.h"
#include "task.h"

/*--------------- Tasks Priority -------------*/
#define DHCP_TASK_PRIO   	( tskIDLE_PRIORITY + 5 )
#define LED_TASK_PRIO    	( tskIDLE_PRIORITY + 1 )
#define TCPECHO_TASK_PRIO 	( tskIDLE_PRIORITY + 2 )
#define IRSND_TASK_PRIO 	( tskIDLE_PRIORITY + 2 )
#define SOLIDSTATE_TASK_PRIO	( tskIDLE_PRIORITY + 2 )


#endif /* TASK_PRIORITIES_H_ */
