#ifndef SPI_H
#define SPI_H

#include <sys/types.h>
#include <reent.h>
#include <string.h>

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * SPI: SPI2, 
 * Transmission: DMA2_Stream7_IRQHandler,
 * CLK	-> PB10
 * SDI	-> PB15
 */
void SPI_init();
//void DMA2_Stream7_IRQHandler(void);
void SPI_send(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
