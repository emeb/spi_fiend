/*
 * tim.c - simple timer-based delay
 * 01-25-19 E. Brombaugh
 */

#include "tim.h"

/*
 * init timer for use
 */
void tim_init(void)
{
    /* enable timer 3 */
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    /* Set up timer for 1usec counting */
    TIM3->CR1 |= TIM_CR1_DIR | TIM_CR1_OPM; // one pulse, down
    TIM3->PSC = 47; // 1us count rate
}

/*
 * delay microsecs
 */
void tim_usleep(uint16_t usec)
{
    TIM3->ARR = usec;   // load delay amount
    TIM3->EGR |= TIM_EGR_UG;  // update
    TIM3->CR1 |= TIM_CR1_CEN; // enable count
    
    /* wait for auto disable */
    while(TIM3->CR1 & TIM_CR1_CEN){}
}
