/*
 * led.c - spi_fiend LED setup
 */

#ifndef __led__
#define __led__

#include "stm32f0xx.h"

#define LED0 (1 << 0) /* port F bit 0 */
#define LED1 (1 << 1) /* port F bit 1 */

void led_init(void);
void led_on(uint32_t LED);
void led_off(uint32_t LED);
void led_toggle(uint32_t LED);

#endif
