/*
 * usrt.c - usrt diag support for f042_usb_spi
 * 11-10-2022 E. Brombaugh
 */

#ifndef __uart__
#define __uart__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx.h"

void uart_init(void);
void uart_putc(void* p, char c);
uint32_t uart_available(void);
char uart_getc(void);
uint8_t uart_set_line_coding(uint32_t bit_rate, uint8_t stop_bits, uint8_t parity,
	uint8_t data_bits);
#ifdef __cplusplus
}
#endif

#endif
