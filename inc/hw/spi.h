#ifndef SPI_H
#define SPI_H

#include <sys/types.h>
#include <reent.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * SPI: SPI2, 
 * Transmission: DMA2_Stream7_IRQHandler,
 * CLK	-> PB10
 * SDI	-> PB15
 */
void SPI_HW_init();
void DMA1_Stream4_IRQHandler ( void );
bool SPI_HW_DMA_send(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
