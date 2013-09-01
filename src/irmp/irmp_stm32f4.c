/*
 * irmp_stm32f4.c
 *
 *  Created on: 01.09.2013
 *      Author: pyro
 */

#include "irmp/irmp.h"
#include "irmp/irsnd.h"
#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"

void IRMP_Init() {
	irmp_init();
	irsnd_init();

	// activate timer interrupt
	TIM_ClearITPendingBit(IRSND_TIMER,TIM_IT_Update);
	TIM_ITConfig(IRSND_TIMER, TIM_IT_Update, ENABLE);
	NVIC_Init(&(NVIC_InitTypeDef) {
			.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn,
			.NVIC_IRQChannelPreemptionPriority= 14,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		});

	// start timer (configured by irsnd_init())
	TIM_Cmd(IRSND_TIMER, ENABLE);
}


void TIM1_UP_TIM10_IRQHandler(){
	if(TIM_GetITStatus(IRSND_TIMER, TIM_FLAG_Update) == SET){
		TIM_ClearITPendingBit(IRSND_TIMER,TIM_IT_Update);

		if(irsnd_is_busy()){
			irsnd_ISR();
		}else{
			irmp_ISR();
		}
	}
}

