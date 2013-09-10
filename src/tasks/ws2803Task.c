
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks/Task_Priorities.h"
#include "tasks/ws2803Task.h"

#include "stm32f4xx.h"
#include "hw/spi.h"

xQueueHandle ws2803Queue;

#define LEDS (6)
static uint8_t ledState[LEDS*3];

void WS2803_Task_init()
{
     SPI_HW_init();

     ws2803Queue = xQueueCreate ( 5, sizeof ( WS2803Command_t ) );

     if ( ws2803Queue == 0 ) {
          printf ( "WS2803 queue creation failed\n" );
     }

     printf("WS2803 init done\n");
     xTaskCreate ( WS2803_thread, ( const signed char * const ) "WS2803_Task",
                   configMINIMAL_STACK_SIZE, NULL, WS2803_TASK_PRIO, NULL );
}

void WS2803_thread ( void *arg )
{
     static WS2803Command_t command;
     for ( ;; ) {


          for ( ;; ) {
               for ( int i=0; i<LEDS*3; i++ ) {
                    ledState[i] = 0xFF;
               }
               // load frame
               while ( !SPI_HW_DMA_send ( ledState,  LEDS * 3 ) ) {
                    vTaskDelay ( 250 );
               }
               // be sure that controller is ready again
               vTaskDelay ( 1500 );
          }

          while ( xQueueReceive ( ws2803Queue, & ( command ), ( portTickType ) portMAX_DELAY ) ) {
               ledState[command.led*3] = command.red;
               ledState[command.led*3+1] = command.green;
               ledState[command.led*3+2] = command.blue;

               // no commands in queues -> start dma transfer
               if ( uxQueueMessagesWaiting ( ws2803Queue ) == 0 )
                    break;
          }
          // load frame
          while ( !SPI_HW_DMA_send ( ledState,  LEDS * 3 ) ) {
               vTaskDelay ( 250 );
          }
          // be sure that controller is ready again
          vTaskDelay ( 5 );
     }
}

