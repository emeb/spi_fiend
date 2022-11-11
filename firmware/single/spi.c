/*
 * spi.h - spi driver for F042_usb_spi
 * 01-23-2019 E. Brombaugh
 */

#include "spi.h"
#include "tim.h"
#include "led.h"

/* macros to define pins */
#define SPI_CS_PIN                        GPIO_PIN_1
#define SPI_CS_GPIO_PORT                  GPIOB
#define SPI_RST_PIN                       GPIO_PIN_4
#define SPI_RST_GPIO_PORT                 GPIOA
#define SPI_DONE_PIN                      GPIO_PIN_1
#define SPI_DONE_GPIO_PORT                GPIOA

/* macros for GPIO setting */
#define SPI_CS_LOW()     HAL_GPIO_WritePin(SPI_CS_GPIO_PORT,SPI_CS_PIN,GPIO_PIN_RESET)
#define SPI_CS_HIGH()    HAL_GPIO_WritePin(SPI_CS_GPIO_PORT,SPI_CS_PIN,GPIO_PIN_SET)
#define SPI_RST_LOW()    HAL_GPIO_WritePin(SPI_RST_GPIO_PORT,SPI_RST_PIN,GPIO_PIN_RESET)
#define SPI_RST_HIGH()   HAL_GPIO_WritePin(SPI_RST_GPIO_PORT,SPI_RST_PIN,GPIO_PIN_SET)
#define SPI_DONE_GET()   HAL_GPIO_ReadPin(SPI_DONE_GPIO_PORT, SPI_DONE_PIN)
#define ICE5_SPI_DUMMY_BYTE   0xFF
#define SPI_SWAP_MOSI()  led_on(LED1)
#define SPI_NORM_MOSI()  led_off(LED1)


/* SPI port handle */
static SPI_HandleTypeDef SpiHandle;

/* flash write state */
uint32_t fpw_addr, fpw_len, fpw_ptr;
uint8_t fpw_buffer[256];

/*
 * get SPI signals on/off the bus
 */
void spi_oe(uint8_t state)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	if(state)
	{
		/* Configure SPI1 CLK/MOSI/MISO as alternate function push-pull */
		GPIO_InitStructure.Pin = GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Alternate = GPIO_AF0_SPI1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else
	{
		/* Configure SPI1 CLK/MOSI/MISO as input /w pullup */
		GPIO_InitStructure.Pin = GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5;
		GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStructure.Pull = GPIO_PULLUP;
		GPIO_InitStructure.Alternate = GPIO_AF0_SPI1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
}

/*
 * Initialize the spi port and associated controls
 */
void spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable GPIO clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Setup GPIO control bits */
	SPI_RST_HIGH();
	SPI_CS_HIGH();
	
	/* CS pin configuration - OD w/ PU to allow multi master */
	GPIO_InitStructure.Pin =  SPI_CS_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SPI_CS_GPIO_PORT, &GPIO_InitStructure);
    
	/* RST pin configuration */
	GPIO_InitStructure.Pin =  SPI_RST_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(SPI_RST_GPIO_PORT, &GPIO_InitStructure);
    
	/* Disable SPI outputs */
	spi_oe(0);
	
	/* Enable the SPI peripheral */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
	SpiHandle.Instance               = SPI1;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.Mode              = SPI_MODE_MASTER;
	HAL_SPI_Init(&SpiHandle);

	/* Enable the SPI peripheral */
    __HAL_SPI_ENABLE(&SpiHandle);
}

/*
 * send a byte and receive a byte via spi
 */
uint8_t spi_txrx(uint8_t data)
{
	/* Wait until the transmit buffer is empty */
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXE) == RESET)
	{
	}

	/* Send the byte */
	*(__IO uint8_t *) ((uint32_t)SPI1+0x0C) = data;
	
	/* Wait to receive a byte*/
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXNE) == RESET)
	{
	}

	/* get the byte read from the SPI bus */ 
	data = *(__IO uint8_t *) ((uint32_t)SPI1+0x0C);
	
	return data;
}

/*
 * Send a block of data bytes via SPI
 */
uint32_t spi_send_data(uint8_t *data, uint32_t sz)
{	
    /* send all bytes */
	while(sz--)
    {
        spi_txrx(*data++);
	}
    
    return 0;
}

/*
 * start the config process
 */
uint8_t spi_config_start(void)
{
    uint8_t retval = 0;
	uint32_t timeout;
	
	/* Enable SPI port onto bus */
	spi_oe(1);
	
	/* drop reset bit */
	SPI_RST_LOW();
	
	/* wait 1us */
	tim_usleep(1);
    
	/* drop CS bit to signal slave mode */
	SPI_CS_LOW();
	
	/* delay >200ns to allow FPGA to recognize reset */
	tim_usleep(200);
    
	/* Wait for done bit to go inactive */
	timeout = 1000;
	while(timeout && (SPI_DONE_GET()==GPIO_PIN_SET))
	{
		timeout--;
	}
	if(!timeout)
	{
		/* Done bit didn't respond to Reset */
		retval = 1;
	}
    
	/* raise reset w/ CS low for SPI SS mode */
	SPI_RST_HIGH();
	
	/* delay >300us to allow FPGA to clear */
	tim_usleep(2000);
    
    return retval;
}

/*
 * finish the config process
 */
uint8_t spi_config_finish(void)
{
    uint8_t retval = 0;
	uint32_t timeout;
	
    /* diagnostic - raise CS for final runout */
	//SPI_CS_HIGH();
    
    /* send clocks while waiting for DONE to assert */
	timeout = 1000;
	while(timeout && (SPI_DONE_GET()==GPIO_PIN_RESET))
	{
		spi_txrx(ICE5_SPI_DUMMY_BYTE);
		timeout--;
	}
	if(!timeout)
	{
		/* FPGA didn't configure correctly */
		retval = 1;
	}
	
	/* send at least 49 more clocks */
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);
	spi_txrx(ICE5_SPI_DUMMY_BYTE);

	/* Raise CS bit for subsequent slave transactions */
	SPI_CS_HIGH();
	
	/* Remove SPI port from bus */
	spi_oe(0);
	
	/* no error handling for now */
	return retval;
}

/*
 * prep for flash access
 */
void spi_flash_access(uint8_t state)
{
	if(state)
	{
		/* set up */
		SPI_RST_LOW();		/* disable the FPGA to get the bus */
		SPI_SWAP_MOSI();	/* swap mosi/miso for flash access */
		spi_oe(1);			/* Enable SPI port onto bus */
	}
	else
	{
		/* teardown */
		spi_oe(0);			/* Remove SPI port from bus */
		SPI_NORM_MOSI();	/* restore mosi/miso order */
		SPI_RST_HIGH();		/* enable the FPGA */
	}
}

/*
 * powerup flash
 */
void spi_flash_powerup(void)
{
	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	/* power up command */
	spi_txrx(0xab);
	
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	
}

/*
 * flash write enable
 */
void spi_flash_wen(void)
{
	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	/* write enable command */
	spi_txrx(0x06);
	
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	
}

/*
 * flash get status command
 */
uint8_t spi_flash_get_status(void)
{
	uint8_t status;
	
	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	/* read status command */
	spi_txrx(0x05);
	status = spi_txrx(0);
	
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	
	return status;
}

/*
 * flash wait for ready 
 */
uint8_t spi_flash_wrdy(void)
{
	int32_t timeout = 100000;
	
	/* loop until BUSY bit is deasserted */
	while((timeout>0) && ((spi_flash_get_status()&0x01) == 0x01))
	{
		timeout--;
	}
	
	return (timeout) ? 0 : 1;
}

/*
 * get ID bytes from flash memory
 */
uint8_t spi_get_flash_id(uint8_t *data)
{
	spi_flash_access(1);	/* enable access */
	
	spi_flash_powerup();	/* power up if FPGA powered the flash down */

	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	/* get ID command */
	spi_txrx(0x9f);
	*data++ = spi_txrx(0);
	*data++ = spi_txrx(0);
	*data++ = spi_txrx(0);
	
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	spi_flash_access(0);	/* disable access */

	return 0;
}

/*
 * erase 32k block of flash
 */
uint8_t spi_flash_erase(uint32_t addr)
{
	uint8_t err = 0;
	
	spi_flash_access(1);	/* enable access */
	
	spi_flash_powerup();	/* power up if FPGA powered the flash down */
	
	spi_flash_wen();		/* write enable */

	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	spi_txrx(0x52);			/* erase 32k command */
	spi_txrx(addr>>16);		/* high 8bit of address */
	spi_txrx(addr>>8);		/* mid 8bit of address */
	spi_txrx(addr);			/* low 8bit of address */
	
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	
	err = spi_flash_wrdy();	/* wait for completion */
	
	spi_flash_access(0);	/* disable access */

	return err;
}

/*
 * start read operation
 */
uint8_t spi_flash_read_start(uint32_t addr)
{
	spi_flash_access(1);	/* enable access */
	
	spi_flash_powerup();	/* power up if FPGA powered the flash down */
	
	SPI_CS_LOW();			/* drop CS bit to access the memory */
	
	spi_txrx(0x03);			/* read command */
	spi_txrx(addr>>16);		/* high 8bit of address */
	spi_txrx(addr>>8);		/* mid 8bit of address */
	spi_txrx(addr);			/* low 8bit of address */
	
	return 0;
}

/*
 * continue read operation
 */
uint8_t spi_flash_read_data(uint8_t *buf, uint32_t len)
{
	while(len--)
	{
		*buf++ = spi_txrx(0);
	}
	
	return 0;
}

/*
 * finish read operation
 */
uint8_t spi_flash_read_finish(void)
{
	SPI_CS_HIGH();			/* Raise CS to disable the memory */
	
	spi_flash_access(0);	/* disable access */

	return 0;
}

/*
 * start write operation
 */
uint8_t spi_flash_write_start(uint32_t addr, uint32_t len)
{
	spi_flash_access(1);	/* enable access */
	
	spi_flash_powerup();	/* power up if FPGA powered the flash down */
	
	/* init write process */
	fpw_addr = addr;
	fpw_len = len;
	fpw_ptr = 0;
	
	return 0;
}

/*
 * continue write operation
 */
uint8_t spi_flash_write_data(uint8_t *buf, uint32_t len)
{
	uint32_t cnt = len, sz;
	uint8_t go = 0;
	
	/* copy data into page buffer */
	while(cnt--)
	{
		/* copy data to page buffer */
		fpw_buffer[fpw_ptr++] = *buf++;
		
		/* check for write condition */
		if(fpw_ptr == 0x100)
		{
			/* page is full */
			sz = 0x100;
			go = 1;
		}
		else if(cnt == 0)
		{
			/* out of data - all done? */
			if((fpw_len - fpw_ptr) == 0)
			{
				/* send partial buffer */
				sz = fpw_ptr;
				go = 1;
			}
		}
		
		/* write? */
		if(go)
		{
			/* wait for ready from previous operation */
			if(spi_flash_wrdy())
			{
				/* timeout error causes premature end */
				return 2;
			}
			
			/* write a page */
			spi_flash_wen();		/* write enable */
		
			SPI_CS_LOW();			/* drop CS bit to access the memory */
			
			spi_txrx(0x02);				/* page write command */
			spi_txrx(fpw_addr>>16);		/* high 8bit of address */
			spi_txrx(fpw_addr>>8);		/* mid 8bit of address */
			spi_txrx(fpw_addr);			/* low 8bit of address */
			
			/* advance state */
			fpw_addr += sz;
			fpw_len -= sz;

			/* send data */
			fpw_ptr = 0;
			while(sz--)
				spi_txrx(fpw_buffer[fpw_ptr++]);
			
			SPI_CS_HIGH();			/* Raise CS to disable the memory */
			
			/* reset */
			fpw_ptr = 0;
			go = 0;
		}
	}
	
	/* done with all data? */ 
	if(fpw_len)
	{
		/* more left */
		return 0;
	}
	else
	{
		/* all done - normal termination */
		spi_flash_access(0);	/* disable access */	
		return 1;
	}
}

