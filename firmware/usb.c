/*
 * usb.c - top-level USB setup
 * 01-23-19 E. Brombaugh
 */

#include "usb.h"
#include "usb_device.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

void usb_init(void)
{
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/* System interrupt init*/
	/* SVC_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SVC_IRQn, 0, 0);
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);

	/* Enable USB pins */
	__HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);

	/* Enable USB device */
	MX_USB_DEVICE_Init();
}

/*
 * @brief This function handles USB global Interrupt / USB wake-up interrupt through EXTI line 18.
 */
void USB_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

