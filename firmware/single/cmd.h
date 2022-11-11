/*
 * cmd.h - serial command handler
 * 05-03-19 E. Brombaugh
 */

#ifndef __cmd__
#define __cmd__

#include "stm32f0xx.h"

void cmd_init(void);
void cmd_proc_bg(uint8_t* Buf, uint32_t *Len);
void cmd_proc_fg(void);

#endif
