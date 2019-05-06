/*
 * led.c - spi_fiend LED setup
 */

#include "led.h"

/*
 * Initialize the breakout board LED
 */
void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* turn on clock for LED GPIO */
	__HAL_RCC_GPIOF_CLK_ENABLE();
	
	/* Enable PF0/PF1 for output */
	GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);
}

/*
 * Turn on LED
 */
void led_on(uint32_t LED)
{
	GPIOF->ODR |= LED;
}

/*
 * Turn off LED
 */
void led_off(uint32_t LED)
{
	GPIOF->ODR &= ~LED;
}

/*
 * Toggle LED
 */
void led_toggle(uint32_t LED)
{
	GPIOF->ODR ^= LED;
}

