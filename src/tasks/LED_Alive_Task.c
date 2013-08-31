/*
 * LED_Alive_Task.c
 *
 *  Created on: 31.08.2013
 *      Author: pyro
 */
#include "tasks/LED_Alive_Task.h"
#include "hw/stm32f4_discovery.h"

// global definied
extern struct netif xnetif;

/**
 * @brief  Toggle Led4 task
 * @param  pvParameters not used
 * @retval None
 */
void LED_ToggleLed_ALIVE(void * pvParameters) {
	while (1) {
		uint32_t test = xnetif.ip_addr.addr;
		/*check if IP address assigned*/
		if (test != 0) {
			for (;;) {
				/* toggle LED4 each 250ms */
				STM_EVAL_LEDToggle(LED2);
				vTaskDelay(250);
			}
		}
	}
}

