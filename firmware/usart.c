/*
 * usart.c - uart diag support for F042
 * 01-23-2019 E. Brombaugh
 */

#include "usart.h"

USART_HandleTypeDef UsartHandle;

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Hang forever */
  while(1)
  {
  }
}

/* USART setup */
void setup_usart(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Setup USART GPIO */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.Pin = GPIO_PIN_2;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = GPIO_AF1_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART Rx as alternate function push-pull */
	//GPIO_InitStructure.Pin = GPIO_PIN_3;
	//GPIO_InitStructure.Pull = GPIO_PULLUP;
	//HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Setup USART */
	__HAL_RCC_USART2_CLK_ENABLE();

	/* USART = 115k-8-N-1 */
	UsartHandle.Instance        = USART2;
	UsartHandle.Init.BaudRate   = 115200;
	UsartHandle.Init.WordLength = USART_WORDLENGTH_8B;
	UsartHandle.Init.StopBits   = USART_STOPBITS_1;
	UsartHandle.Init.Parity     = USART_PARITY_NONE;
	UsartHandle.Init.Mode       = USART_MODE_TX_RX;
	if(HAL_USART_DeInit(&UsartHandle) != HAL_OK)
	{
		Error_Handler();
	}  
	if(HAL_USART_Init(&UsartHandle) != HAL_OK)
	{
		Error_Handler();
	}
}

/*
 * output for tiny printf
 */
void usart_putc(void* p, char c)
{
	while(__HAL_USART_GET_FLAG(&UsartHandle, USART_FLAG_TC) == RESET)
	{
	}
	USART2->TDR = c;
}
