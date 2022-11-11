/*
 * usart.c - usart diag support for f042_usb_spi
 * 01-23-2019 E. Brombaugh
 */

#ifndef __usart__
#define __usart__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx.h"

void setup_usart(void);
void usart_putc(void* p, char c);
uint32_t usart_available(void);
char usart_getc(void);

#ifdef __cplusplus
}
#endif

#endif
