#include "hw/spi.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_dma.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "FreeRTOS.h"
#include "semphr.h"

#define SPI_TX_DMA	(DMA1_Stream4)

volatile static struct {
     DMA_InitTypeDef spiTXDMA;
     bool dmaRunning;
} spi_state;

static inline void startSPIDMAFromBuffer(const uint8_t* data, size_t len)
{
     // copy message
     uint8_t* str = ( uint8_t* ) malloc ( len * sizeof ( uint8_t ) );
     memcpy(str, data, len * sizeof(uint8_t));
     
     spi_state.spiTXDMA.DMA_Memory0BaseAddr = ( uint32_t ) str;
     spi_state.spiTXDMA.DMA_BufferSize = len;

     // start transfer
     // DMA Request: Always call DMA_Init and DMA_CMD
     DMA_Init ( SPI_TX_DMA, ( DMA_InitTypeDef* ) &spi_state.spiTXDMA );
     DMA_Cmd ( SPI_TX_DMA, ENABLE );
     spi_state.dmaRunning = true;
}

void DMA1_Stream4_IRQHandler ( void )
{
     DMA_Cmd ( SPI_TX_DMA, DISABLE );
     DMA_ClearITPendingBit ( SPI_TX_DMA, DMA_IT_TCIF4 );
  
     vPortEnterCritical();
     {
	spi_state.dmaRunning = false;
     }
     vPortExitCritical();
}

bool SPI_HW_DMA_send(const uint8_t* data, size_t len){
     bool succStart = false;
     vPortEnterCritical();
     {
	if(!spi_state.dmaRunning){
	  startSPIDMAFromBuffer(data, len);
	  succStart = true;
	  spi_state.dmaRunning = true;
	}
     }
     vPortExitCritical();
     return succStart;
}

/**
 * Initialize SPI.
 */
void SPI_HW_init()
{
     // Enable peripheral clocks
     RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
     RCC->APB1ENR |= RCC_APB1Periph_SPI2;

     // Initialize Serial Port
     GPIO_Init ( GPIOB, & ( GPIO_InitTypeDef ) {
          .GPIO_Pin = ( GPIO_Pin_10 | GPIO_Pin_15 ),
           .GPIO_Speed = GPIO_Speed_50MHz, .GPIO_Mode = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP
     } );

     GPIO_PinAFConfig ( GPIOB, GPIO_PinSource10, GPIO_AF_SPI2 );
     GPIO_PinAFConfig ( GPIOB, GPIO_PinSource15, GPIO_AF_SPI2 );
   
     // SPI Init
     SPI_Init(SPI2, &(SPI_InitTypeDef){
	.SPI_Direction = SPI_Direction_1Line_Tx,
	.SPI_Mode = SPI_Mode_Master,
	.SPI_DataSize = SPI_DataSize_8b,
	.SPI_CPOL = SPI_CPOL_Low,
	.SPI_CPHA = SPI_CPHA_1Edge,
	.SPI_NSS = SPI_NSS_Soft,
	.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
	.SPI_FirstBit = SPI_FirstBit_MSB,
	.SPI_CRCPolynomial = 7
    });
     
     // ENABLE DMA
     SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
     SPI_Cmd(SPI2, ENABLE);

      // Enable DMA
     RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_DMA1, ENABLE );
     DMA_DeInit ( SPI_TX_DMA );
     DMA_StructInit ( ( DMA_InitTypeDef* ) &spi_state.spiTXDMA );

     // configure dma
     spi_state.spiTXDMA.DMA_Channel = DMA_Channel_0;
     spi_state.spiTXDMA.DMA_DIR = DMA_DIR_MemoryToPeripheral;
     spi_state.spiTXDMA.DMA_PeripheralBaseAddr = ( uint32_t ) & ( SPI2->DR );
     spi_state.spiTXDMA.DMA_MemoryInc = DMA_MemoryInc_Enable;
     spi_state.spiTXDMA.DMA_Memory0BaseAddr = ( uint32_t ) 0;

     // Enable DMA finished interrupt
     DMA_ITConfig ( SPI_TX_DMA, DMA_IT_TC, ENABLE );
     DMA_ClearITPendingBit ( SPI_TX_DMA, DMA_IT_TCIF4 );

     // Enable DMA Transfer finished interrupt for DMA
     NVIC_Init ( & ( NVIC_InitTypeDef ) {
          .NVIC_IRQChannel = DMA1_Stream4_IRQn,
           .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_KERNEL_INTERRUPT_PRIORITY-1,
                .NVIC_IRQChannelSubPriority = 1, .NVIC_IRQChannelCmd =
                          ENABLE
     } );
}
