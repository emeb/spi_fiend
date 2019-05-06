/*
 * main.h - top level for spi_fiend - a USB to SPI interface
 * 03-13-19 E. Brombaugh
 */
 
#include "stm32f0xx_hal.h"
#include "printf.h"
#include "usart.h"
#include "tim.h"
#include "led.h"
#include "spi.h"
#include "usb.h"
#include "cmd.h"

extern volatile uint8_t cmd_state, cmd_err;

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

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;
  RCC_CRSInitTypeDef RCC_CRSInitStruct;

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
 * main entry pt
 */
int main(void)
{
    uint8_t prev_cmd_state;
    
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* set up clocks */
	SystemClock_Config();
	
	/* init the UART for diagnostics */
	setup_usart();
	init_printf(0,usart_putc);
	printf("\n\n\rspi_fiend\n\r");
	printf("\n");
	printf("SYSCLK = %d\n\r", HAL_RCC_GetSysClockFreq());
	printf("\n");
	
	/* init usec timer */
	tim_init();
	printf("Microsecond timer initialized\n\r");

	/* init LED */
	led_init();
	printf("LED initialized\n\r");

	/* init SPI */
	spi_init();
	printf("SPI initialized\n\r");
	
	/* init command processor */
	cmd_init();
	printf("Command Processor initialized\n\r");
	
	/* init USB */
	usb_init();
	printf("USB initialized\n\r");
    prev_cmd_state = cmd_state;
    
	/* wait here */
	printf("Looping\n\r");
	while (1)
	{
		/* run the foreground cmd processor */
		cmd_proc_fg();
		
		/* state changes? */
		if(prev_cmd_state != cmd_state)
		{
			prev_cmd_state = cmd_state;
			printf("State = %d, err = %d\n\r", cmd_state, cmd_err);
			
			if(prev_cmd_state)
			{
				led_off(LED0);
			}
			else
			{
				if(cmd_err)
					led_on(LED0);
			}
		}
	}
}

/*
 * handle System tick timer.
 */
void SysTick_Handler(void)
{
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
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
