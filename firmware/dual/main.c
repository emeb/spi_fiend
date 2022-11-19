/*
 * main.h - top level for f042_tinyusb spi_fiend
 * 11-09-22 E. Brombaugh
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "stm32f0xx_hal.h"
#include "main.h"
#include "printf.h"
#include "uart.h"
#include "tim.h"
#include "led.h"
#include "tusb.h"
#include "spi.h"
#include "cmd.h"

/* build version in simple format */
const char *fwVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * error
 */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}

/*
 * System Clock Configuration
 */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Enable the SYSCFG APB clock 
    */
  __HAL_RCC_CRS_CLK_ENABLE();

    /**Configures CRS 
    */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/*
 * set up F042 USB
 */
void f042_usb_config(void)
{
	/* Enable USB pins */
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);

	// USB Pins
	__HAL_RCC_GPIOA_CLK_ENABLE();
	// Configure USB DM and DP pins. This is optional, and maintained only for user guidance.
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// USB Clock enable
	__HAL_RCC_USB_CLK_ENABLE();
}

/*
 * foreground CDC handler
 */
static void cdc_task(void)
{
	uint8_t buf[64], *bptr;
	uint32_t count;
	
	/* handle the SPI port commands */
	if(tud_cdc_n_available(CDC_ITF_SPI))
	{
        count = tud_cdc_n_read(CDC_ITF_SPI, buf, sizeof(buf));
		cmd_proc(buf, count);
	}
	
	/* handle SPI port readback */
	cmd_proc_flash_read();

	/* handle UART output */
	if(tud_cdc_n_available(CDC_ITF_UART))
	{
        count = tud_cdc_n_read(CDC_ITF_UART, buf, sizeof(buf));
		bptr = buf;
		while(count--)
		{
			uart_putc(NULL, *bptr++);
		}
	}
	
	/* handle UART input */
	if(uart_available())
	{
		buf[0] = uart_getc();
		tud_cdc_n_write(CDC_ITF_UART, buf, 1);
		tud_cdc_n_write_flush(CDC_ITF_UART);
	}
}

/*
 * Handle Line coding change
 */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
	/* only care about the UART interface */
	if(itf != CDC_ITF_UART)
		return;
	
	/*
	 * contents of cdc_line_coding_t struct:
	 * uint32_t bit_rate;
	 * uint8_t  stop_bits; ///< 0: 1 stop bit - 1: 1.5 stop bits - 2: 2 stop bits
	 * uint8_t  parity;    ///< 0: None - 1: Odd - 2: Even - 3: Mark - 4: Space
	 * uint8_t  data_bits; ///< can be 5, 6, 7, 8 or 16
     */
	uart_set_line_coding(p_line_coding->bit_rate,
		p_line_coding->stop_bits, p_line_coding->parity, p_line_coding->data_bits);
}

/*
 * main entry pt
 */
int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* set up clocks */
	SystemClock_Config();
	
	/* init the UART for diagnostics */
	uart_init();
#if 0
	init_printf(0,uart_putc);
	printf("\n\n\rf042_tinyusb spi_fiend %s starting...\n\r", fwVersionStr);
	printf("\n");
	printf("SYSCLK = %d\n\r", HAL_RCC_GetSysClockFreq());
	printf("\n");
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);
	printf("\n");
#endif
	
	/* init usec timer */
	tim_init();

	/* init LED */
	led_init();
	
	/* init SPI */
	spi_init();
	
	/* init command processor */
	cmd_init();
	
	/* init TinyUSB */
	f042_usb_config();
	tud_init(BOARD_TUD_RHPORT);

	/* wait here */
	while(1)
	{
		tud_task();
		cdc_task();
	}
}

/*
 * handle System tick timer.
 */
void SysTick_Handler(void)
{
	HAL_IncTick();
}

/*
 * handle USB
 */
void USB_IRQHandler(void)
{
	tud_int_handler(0);
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif
