
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <../../../FreeRTOS-Plus/Demo/FreeRTOS_Plus_UDP_and_CLI_LPC1830_GCC/ThirdParty/USB_CDC/include/usb.h>

#include "tasks/Task_Priorities.h"
#include "tasks/ws2803Task.h"

#include "stm32f4xx.h"
#include "hw/spi.h"

#include <stdbool.h>

xQueueHandle ws2803Queue;

#define LEDS (6)
static uint8_t ledState[LEDS*3];


#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

int highs = 0;

#define WS_HW_SPI

#ifdef WS_HW_SPI
#define WS_SPI_INIT()		SPI_HW_init()
#define WS_SPI_SEND(data, len)	SPI_HW_DMA_send(data, len)
#else

#define WS_SPI_INIT()		WS2803_Lowlevel_init()
#define WS_SPI_SEND(data, len)	WS2803_SPI_send(data, len)

#define CLK_LOW()		GPIO_ResetBits(GPIOB, GPIO_Pin_10)	
#define CLK_HIGH()		{ \
				    GPIO_SetBits(GPIOB, GPIO_Pin_10); \
				    ++highs; \
				}

#define SDI_LOW()		GPIO_ResetBits(GPIOB, GPIO_Pin_15)	
#define SDI_HIGH()		GPIO_SetBits(GPIOB, GPIO_Pin_15)


void WS2803_Lowlevel_init(){
     // Enable peripheral clocks
     RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;

     // Initialize Serial Port
     GPIO_Init ( GPIOB, & ( GPIO_InitTypeDef ) {
          .GPIO_Pin = ( GPIO_Pin_10 | GPIO_Pin_15 ),
           .GPIO_Speed = GPIO_Speed_100MHz, .GPIO_Mode = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP, 
	    .GPIO_PuPd = GPIO_PuPd_NOPULL
     } );
  
    // Reset bits
    GPIO_ResetBits(GPIOB, GPIO_Pin_10);
    GPIO_ResetBits(GPIOB, GPIO_Pin_15);
}

void WS2803_SPI_send_byte(uint8_t data){
  for(int i=0; i<7;i++){
    if(data & (1<<i)){
      SDI_HIGH();
    }else{
      SDI_LOW();
    }
    CLK_LOW();
    for(int j=0;j<10;j++)
      asm volatile ("NOP");
    CLK_HIGH();
    for(int j=0;j<10;j++)
      asm volatile ("NOP");
  }
}

bool WS2803_SPI_send(const uint8_t* data, size_t len){
  vPortEnterCritical();
  highs = 0;
  CLK_LOW();
  delay_us ( 600 );
  for(size_t i=0; i< len; i++)
    WS2803_SPI_send_byte(data[i]);
  CLK_LOW();
  vPortExitCritical();
  return true;
}
#endif

void WS2803_Task_init()
{
     WS_SPI_INIT();

     ws2803Queue = xQueueCreate ( 5, sizeof ( WS2803Command_t ) );

     if ( ws2803Queue == 0 ) {
          printf ( "WS2803 queue creation failed\n" );
     }

     // deactivate all leds
     for ( int i=0; i< LEDS*3; i++ ) {
          ledState[i] = ( 0xFF );
     }
     // all leds off
     //while ( !WS2803_SPI_send ( ledState,  LEDS * 3 ));
     
     // message and start thread
     printf ( "WS2803 init done\n" );
     xTaskCreate ( WS2803_thread, ( const signed char * const ) "WS2803_Task",
                   configMINIMAL_STACK_SIZE, NULL, WS2803_TASK_PRIO, NULL );
}

void WS2803_thread ( void *arg )
{
     static WS2803Command_t command;
     
     while ( !WS_SPI_SEND ( ledState,  LEDS * 3 ));
     vTaskDelay(5000);
     
     for ( ;; ) {

	  uint8_t i =  0x00;
          for ( ;; ) {
		
	       ledState[17] =  i;
	       for(int j=0;j<17;j++)
		ledState[j] =  0xFF - i;
               //ledState[16] =  i;
	       //ledState[15] =  i;

               // load frame
               while ( !WS_SPI_SEND ( ledState,  LEDS * 3 ) ) {
                    vTaskDelay ( 250 );
               }
               // be sure that controller is ready again
               vTaskDelay ( 500 );
	       i+=2;
	       printf("Value set %x -> Highs: %d\n",(int) i, highs);
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
          while ( !WS_SPI_SEND ( ledState,  LEDS * 3 ) ) {
               vTaskDelay ( 250 );
          }
          // be sure that controller is ready again
          vTaskDelay ( 5 );
     }
}

