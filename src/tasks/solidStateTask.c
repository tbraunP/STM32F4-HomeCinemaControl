
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks/Task_Priorities.h"
#include "tasks/solidStateTask.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

#include "tasks/command_dispatcher.h"

xQueueHandle solidStateQueue;

static uint16_t device2pin[] = { GPIO_Pin_7, GPIO_Pin_8, GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11};
static SolidStateRelais_Mode_t pinState[] = { SR_OFF, SR_OFF, SR_OFF, SR_OFF, SR_OFF};
static uint8_t devicePins = 5;

void SolidState_Task_LowLevel_init(){
     RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOD, ENABLE );

     /* GPIO Configuration */
     GPIO_Init ( GPIOD, & ( GPIO_InitTypeDef ) {
          .GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10 | GPIO_Pin_9 | GPIO_Pin_8 | GPIO_Pin_7,
           .GPIO_Mode = GPIO_Mode_OUT,
            .GPIO_Speed = GPIO_Speed_50MHz,
             .GPIO_OType = GPIO_OType_PP,
              .GPIO_PuPd = GPIO_PuPd_NOPULL
     } );
     
     // set all ports to low
     for(uint8_t i = 0; i < devicePins; i++){
      GPIO_ResetBits(GPIOD, device2pin[i]);
     }
          
     printf("SolidState_Task lowlevel init done\n");
}

void SolidState_Task_init(){
     SolidState_Task_LowLevel_init();
     
     solidStateQueue = xQueueCreate(10, sizeof(Command_t));
     
     if(solidStateQueue == 0){
       printf("SolidState queue creation failed\n");
     }

     xTaskCreate ( SolidState_thread, ( const signed char * const ) "SolidState_Task",
                   configMINIMAL_STACK_SIZE, NULL, SOLIDSTATE_TASK_PRIO, NULL );
}

void SolidState_thread ( void *arg ){
  static Command_t incoming;
  static SolidStateCommand_t* command;
  
  for(;;){
     if( xQueueReceive( solidStateQueue, &( incoming ), ( portTickType ) portMAX_DELAY ) ){
        command = incoming.solidStateCommands;
	if(pinState[command->relais] == command->newState)
	  continue;
       
	// change pin state
	pinState[command->relais] = command->newState;
	if(command->newState == SR_ON){
	  GPIO_SetBits(GPIOD, device2pin[command->relais]);
	}else{
	  GPIO_ResetBits(GPIOD, device2pin[command->relais]);
	}
	// free payload
	free(incoming.raw);
    }
  }
}

SolidStateRelais_Mode_t SolidState_getState(SolidStateRelais_t relais){
  return pinState[relais];
}
