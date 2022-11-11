/*
 * cmd.c - serial command handler
 * 05-03-19 E. Brombaugh
 */

#include "cmd.h"
#include "main.h"
#include "tusb.h"
#include "spi.h"

enum cmd_states
{
	CMD_WAIT,
	CMD_S_HEAD,
	CMD_S_BODY,
	CMD_E_HEAD,
	CMD_R_HEAD,
	CMD_W_HEAD,
	CMD_W_BODY,
};

enum cmd_fg_states
{
	CMD_FG_IDLE,
	CMD_FG_READ,
};

volatile uint8_t cmd_state, cmd_cmd, cmd_err, cmd_fg_state;
uint32_t cmd_len, cmd_cnt, cmd_addr;

/*
 * init command handler
 */
void cmd_init(void)
{
	cmd_state = CMD_WAIT;
	cmd_err = 0;
	cmd_fg_state = CMD_FG_IDLE;
}

/*
 * input process for the serial command stream
 */
void cmd_proc(uint8_t* Buf, uint32_t Len)
{
	uint32_t cnt, sz;
	uint8_t replybuf[4];
	
	/* discard RX data if FG is not idle */
	if(cmd_fg_state != CMD_FG_IDLE)
		return;
	
	/* process serial */
	cnt = 0;
	while(cnt < Len)
	{
		/* process buffer */
		switch(cmd_state)
		{
			case CMD_WAIT:
				/* Wait for command */
				switch(Buf[cnt++])
				{
					case 's': /* s command => FPGA spi load */
						/* advance to Head state */
						cmd_state = CMD_S_HEAD;
						cmd_cnt = 4;
						cmd_len = 0;
						cmd_err = 0;
						break;
					
					case 'i': /* i command => get Flash ID */
						cmd_err = spi_get_flash_id(replybuf);
						replybuf[3] = cmd_err;
						tud_cdc_n_write(CDC_ITF_SPI, replybuf, 4);
						tud_cdc_n_write_flush(CDC_ITF_SPI);
						break;
					
					case 'e': /* e command => erase 32k block of flash */
						cmd_state = CMD_E_HEAD;
						cmd_cnt = 4;
						cmd_addr = 0;
						cmd_err = 0;
						break;
					
					case 'r': /* r command => read flash memory */
						cmd_state = CMD_R_HEAD;
						cmd_cnt = 8;
						cmd_addr = 0;
						cmd_len = 0;
						cmd_err = 0;
						break;
					
					case 'w': /* w command => write flash memory */
						cmd_state = CMD_W_HEAD;
						cmd_cnt = 8;
						cmd_addr = 0;
						cmd_len = 0;
						cmd_err = 0;
						break;
					
					default: /* unsupported command - just consume bytes */
						break;
 				}
				break;
			
			case CMD_S_HEAD:
				/* process header for s command */
				if(cmd_cnt-- > 0)
				{
					/* assemble 4 bytes to length */
					cmd_len = (cmd_len<<8) | Buf[cnt++];
				}
				else
				{
					/* start configuration and send remaining buffer */
					if(spi_config_start())
						cmd_err |= 1;
					
					sz = Len-cnt;
                    if(sz < cmd_len)
                    {
                        /* announced data length is more than remaining */
                        spi_send_data(&Buf[cnt], sz);
                        
                        /* advance to Body state */
                        cmd_state = CMD_S_BODY;
                        cmd_len -= sz;
                        cnt = Len;
                    }
                    else
                    {
                        /* announced data length is less than remaining */
                        spi_send_data(&Buf[cnt], cmd_len);
                        if(spi_config_finish())
							cmd_err |= 2;
                        
                        /* return to Wait */
                        cnt += cmd_len;
                        cmd_len = 0;
						
						/* send status */
						replybuf[0] = cmd_err;
						tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
						tud_cdc_n_write_flush(CDC_ITF_SPI);
						
                        cmd_state = CMD_WAIT;
                    }
				}
				break;
				
			case CMD_S_BODY:
				/* process body for s command */
				if(Len < cmd_len)
				{
					/* continue sending */
					spi_send_data(Buf, Len);
					cmd_len -= Len;
					cnt = Len;
				}
				else
				{
					/* finish sending and return to Wait state */
                    /* if Len != cmd_len then tty added some junk! */
					spi_send_data(Buf, cmd_len);
					if(spi_config_finish())
						cmd_err |= 2;
					if(Len != cmd_len)
						cmd_err |= 4;
					cnt += cmd_len;
					cmd_len = 0;

					/* send status */
					replybuf[0] = cmd_err;
					tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
					tud_cdc_n_write_flush(CDC_ITF_SPI);
						
					cmd_state = CMD_WAIT;
				}
				break;
				
			case CMD_E_HEAD:
				/* process header for e command */
				/* assemble 4 bytes to address */
				cmd_addr = (cmd_addr<<8) | Buf[cnt++];
				cmd_cnt--;
			
				/* after addr recv'd then execute */
				if(cmd_cnt == 0)
				{
					/* handle start erase command */
					cmd_err = spi_flash_erase(cmd_addr);
					
					/* send status */
					replybuf[0] = cmd_err;
					tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
					tud_cdc_n_write_flush(CDC_ITF_SPI);
					
					/* return to wait state */
					cmd_state = CMD_WAIT;
				}
				break;
			
			case CMD_R_HEAD:
				/* process header for r command */
				/* assemble 8 bytes to address & length */
				if(cmd_cnt >4)
					cmd_addr = (cmd_addr<<8) | Buf[cnt++];
				else
					cmd_len = (cmd_len<<8) | Buf[cnt++];
				cmd_cnt--;
			
				/* after addr & len recv'd then begin read */
				if(cmd_cnt == 0)
				{
					/* set up for read */
					cmd_err = spi_flash_read_start(cmd_addr);
					
					/* kick off foreground data handler */
					cmd_fg_state = CMD_FG_READ;
		
					/* return to wait state */
					cmd_state = CMD_WAIT;
				}
				break;
			
			case CMD_W_HEAD:
				/* process header for w command */
				/* assemble 8 bytes to address & length */
				if(cmd_cnt >4)
					cmd_addr = (cmd_addr<<8) | Buf[cnt++];
				else
					cmd_len = (cmd_len<<8) | Buf[cnt++];
				cmd_cnt--;
			
				/* after addr & len recv'd then execute write */
				if(cmd_cnt == 0)
				{
					/* set up for read */
					cmd_err = spi_flash_write_start(cmd_addr, cmd_len);
					
					/* return to wait state */
					cmd_state = CMD_W_BODY;
					
					/* send remaining data to flash */
					sz = Len-cnt;
					if(spi_flash_write_data(&Buf[cnt], sz))
					{
						/* write complete so send status */
						replybuf[0] = cmd_err & 2;
						tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
						tud_cdc_n_write_flush(CDC_ITF_SPI);
						
						/* return to wait state */
                        cmd_state = CMD_WAIT;
					}
					else
					{
                        /* write not done so advance to Body state */
                        cmd_state = CMD_W_BODY;
                    }
					
					/* all data has been consumed */
					cnt = Len;
				}
				break;
							
			case CMD_W_BODY:
				/* process body for w command */
				if(spi_flash_write_data(&Buf[cnt], Len))
				{
					/* write complete so send status */
					replybuf[0] = cmd_err & 2;
					tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
					tud_cdc_n_write_flush(CDC_ITF_SPI);
					
					/* return to wait state */
					cmd_state = CMD_WAIT;
				}
				
				/* all data has been consumed */
				cnt = Len;
				break;
			
			default:
				/* return to Wait state */
				cmd_state = CMD_WAIT;
				break;
		}
	}
}

/*
 * handle body of flash read process
 */
void cmd_proc_flash_read(void)
{
	if(cmd_fg_state == CMD_FG_READ)
	{
		/* how much room in the write buffer */
		uint32_t avail = tud_cdc_n_write_available(CDC_ITF_SPI);
		if(avail)
		{
			uint8_t replybuf[64];
			uint32_t len;
			
			/* send data? */
			if(cmd_len)
			{
				/* get data from flash max(64, avail, cmd_len) */
				len = (avail > 64) ? 64 : avail;
				len = (cmd_len > len) ? len : cmd_len;
				cmd_err |= spi_flash_read_data(replybuf, len);
			
				/* send to host */
				tud_cdc_n_write(CDC_ITF_SPI, replybuf, len);
				cmd_len -= len;
			}
			
			/* finish? */
			if(cmd_len == 0)
			{
				/* wait for space available */
				if(tud_cdc_n_write_available(CDC_ITF_SPI))
				{
					/* finish read */
					cmd_err |= spi_flash_read_finish();
					
					/* send status */
					replybuf[0] = cmd_err;
					len = tud_cdc_n_write(CDC_ITF_SPI, replybuf, 1);
					tud_cdc_n_write_flush(CDC_ITF_SPI);
					
					/* Finished, return to idle */
					cmd_fg_state = CMD_FG_IDLE;
				}
			}
		}
	}
}

