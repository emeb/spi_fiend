/*
 * uart.c - uart diag support for F042
 * 11-10-2022 E. Brombaugh
 */

#include "uart.h"
#include <string.h>

UART_HandleTypeDef UartHandle;

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
void uart_init(void)
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
	GPIO_InitStructure.Pin = GPIO_PIN_3;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Setup USART */
	__HAL_RCC_USART2_CLK_ENABLE();

	/* UART = 115k-8-N-1 */
	UartHandle.Instance        = USART2;
	UartHandle.Init.BaudRate   = 115200;
	UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits   = UART_STOPBITS_1;
	UartHandle.Init.Parity     = UART_PARITY_NONE;
	UartHandle.Init.Mode       = UART_MODE_TX_RX;
	if(HAL_UART_DeInit(&UartHandle) != HAL_OK)
	{
		Error_Handler();
	}  
	if(HAL_UART_Init(&UartHandle) != HAL_OK)
	{
		Error_Handler();
	}
}

/*
 * output for tiny printf
 */
void uart_putc(void* p, char c)
{
	while(__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET)
	{
	}
	UartHandle.Instance->TDR = c;
}

/*
 * check if input ready
 */
uint32_t uart_available(void)
{
	return (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_RXNE) == SET) ? 1 : 0;
}

/*
 * get input
 */
char uart_getc(void)
{
	return UartHandle.Instance->RDR;
}

/*
 * set line coding
 */
uint8_t uart_set_line_coding(uint32_t bit_rate, uint8_t stop_bits, uint8_t parity,
	uint8_t data_bits)
{
	UART_InitTypeDef ui;
	
	/* duplicate the init */
	memcpy(&ui, &UartHandle.Init, sizeof(UART_InitTypeDef));
	
	/* set up the baud rate */
	ui.BaudRate = bit_rate;
	
	/* set up the word size */
	if(data_bits == 7)
		ui.WordLength = UART_WORDLENGTH_7B;
	else if(data_bits == 8)
		ui.WordLength = UART_WORDLENGTH_8B;
	else
		return 1;
	
	/* set the stop bits */
	if(stop_bits == 0)
		ui.StopBits   = UART_STOPBITS_1;
	else if(stop_bits == 2)
		ui.StopBits   = UART_STOPBITS_2;
	else
		return 2;
	
	/* set the parity */
	if(parity == 0)
		ui.Parity     = UART_PARITY_NONE;
	else if(parity == 1)
	{
		ui.Parity     = UART_PARITY_ODD;
		if(ui.WordLength == UART_WORDLENGTH_7B)
			ui.WordLength = UART_WORDLENGTH_8B;
		else
			ui.WordLength = UART_WORDLENGTH_9B;
	}
	else if(parity == 2)
	{
		ui.Parity     = UART_PARITY_EVEN;
		if(ui.WordLength == UART_WORDLENGTH_7B)
			ui.WordLength = UART_WORDLENGTH_8B;
		else
			ui.WordLength = UART_WORDLENGTH_9B;
	}
	else
		return 3;
	
	/* copy modified handle back */
	memcpy(&UartHandle.Init, &ui, sizeof(UART_InitTypeDef));
	
	/* reset configuration */
	HAL_UART_DeInit(&UartHandle);
	HAL_UART_Init(&UartHandle);
	
	return 0;
}
