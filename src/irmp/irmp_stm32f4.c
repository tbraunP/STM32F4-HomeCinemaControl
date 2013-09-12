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

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

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
	
//	// Debug PIN 
//        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

        /* GPIO Configuration */
//        GPIO_Init(GPIOD, & (GPIO_InitTypeDef) {
//	  .GPIO_Pin = GPIO_Pin_0,
//	  .GPIO_Mode = GPIO_Mode_OUT, 
//	  .GPIO_Speed = GPIO_Speed_25MHz,
//	  .GPIO_OType = GPIO_OType_PP,
//	  .GPIO_PuPd = GPIO_PuPd_NOPULL
//	});
}


void TIM1_UP_TIM10_IRQHandler(){
	if(TIM_GetITStatus(IRSND_TIMER, TIM_FLAG_Update) == SET){
		TIM_ClearITPendingBit(IRSND_TIMER,TIM_IT_Update);

		if(irsnd_is_busy()){
			irsnd_ISR();
		}else{
			//GPIO_ToggleBits(GPIOD, GPIO_Pin_0);		
			irmp_ISR();
			//GPIO_ToggleBits(GPIOD, GPIO_Pin_0);	
		}
	}
}

