
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks/Task_Priorities.h"
#include "tasks/solidStateTask.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

#include "tasks/systemStateWatcher.h"
#include "tasks/command_dispatcher.h"

xQueueHandle solidStateQueue;

static uint16_t device2pin[] = { GPIO_Pin_7, GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11};
static SolidStateRelais_Mode_t pinState[] = { SR_OFF, SR_OFF, SR_OFF, SR_OFF};
static uint8_t devicePins = 4;


void SolidState_Task_LowLevel_init()
{
     RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOD, ENABLE );

     /* GPIO Configuration */
     GPIO_Init ( GPIOD, & ( GPIO_InitTypeDef ) {
          .GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10 | GPIO_Pin_9 | GPIO_Pin_7,
           .GPIO_Mode = GPIO_Mode_OUT,
            .GPIO_Speed = GPIO_Speed_50MHz,
             .GPIO_OType = GPIO_OType_PP,
              .GPIO_PuPd = GPIO_PuPd_NOPULL
     } );

     // set all ports to low
     for ( uint8_t i = 0; i < devicePins; i++ ) {
          GPIO_ResetBits ( GPIOD, device2pin[i] );
     }

     //printf ( "SolidState_Task lowlevel init done\n" );
}

void SolidState_sendStatus ( SolidStateRelais_t relais, SolidStateRelais_Mode_t mode )
{
     static Status_Update_t status;
     status.key.fromComponent = SOLIDSTATE;
     status.key.uuid = relais;

     // create message
     status.payload.solidStateStatus = malloc ( sizeof ( SolidStateCommand_t ) );
     status.payload.solidStateStatus->relais = relais;
     status.payload.solidStateStatus->newState = mode;

     // send message
     SystemStateWatcher_Enqueue ( &status );
}

void SolitState_statusFullDump()
{
     for ( int i=0; i< SOLIDSTATE_RELAIS; i++ )
          SolidState_sendStatus ( i, SR_OFF );
}

/*
 * Thread for solidState relais handling
 */
void SolidState_thread ( void *arg )
{
     static Command_t incoming;
     static SolidStateCommand_t* command;

     for ( ;; ) {
          if ( xQueueReceive ( solidStateQueue, & ( incoming ), ( portTickType ) portMAX_DELAY ) ) {
               command = incoming.payload.solidStateCommands;
               if ( pinState[command->relais] == command->newState )
                    continue;

               // send status message
               SolidState_sendStatus ( command->relais, command->newState );

               // change pin state
               pinState[command->relais] = command->newState;
               if ( command->newState == SR_ON ) {
                    GPIO_SetBits ( GPIOD, device2pin[command->relais] );
               } else {
                    GPIO_ResetBits ( GPIOD, device2pin[command->relais] );
               }
               // free payload
               free ( incoming.payload.raw );
          }
     }
}


void SolidState_Task_init()
{
     SolidState_Task_LowLevel_init();

     solidStateQueue = xQueueCreate ( 10, sizeof ( Command_t ) );

     if ( solidStateQueue == 0 ) {
          printf ( "SolidState queue creation failed\n" );
     }

     xTaskCreate ( SolidState_thread, ( const signed char * const ) "SolidState_Task",
                   configMINIMAL_STACK_SIZE, NULL, SOLIDSTATE_TASK_PRIO, NULL );
}
