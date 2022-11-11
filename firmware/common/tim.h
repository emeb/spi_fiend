/*
 * tim.h - simple timer-based delay
 * 01-25-19 E. Brombaugh
 */

#ifndef __tim__
#define __tim__

#include "stm32f0xx_hal.h"

void tim_init(void);
void tim_usleep(uint16_t usec);

#endif
