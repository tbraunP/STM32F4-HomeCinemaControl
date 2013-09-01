/*
 * irmp_stm32f4.h
 *
 *  Created on: 01.09.2013
 *      Author: pyro
 */

#ifndef IRMP_STM32F4_H_
#define IRMP_STM32F4_H_

#ifdef __cplusplus
extern "C" {
#endif

void IRMP_Init();

void TIM1_UP_TIM10_IRQHandler();

#ifdef __cplusplus
}
#endif


#endif /* IRMP_STM32F4_H_ */
