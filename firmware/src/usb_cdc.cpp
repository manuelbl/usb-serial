/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB CDC Implementation
 */

#include "common.h"
#include "hardware.h"
#include "usb_cdc.h"
#include "usb_conf.h"
#include "usb_serial.h"
#include <libopencm3/cm3/nvic.h>
#if defined(STM32F0)
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/syscfg.h>
#endif
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
#include <string.h>

#define USB_CDC_REQ_GET_LINE_CODING 0x21

usbd_device *usb_device;

static uint16_t configured;

static enum usbd_request_return_codes cdc_control_request(
	usbd_device *dev,
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	usbd_control_complete_callback *complete)
{
	switch (req->bRequest)
	{
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;

		if (req->wIndex != 0)
			return USBD_REQ_NOTSUPP;

		usb_serial.set_line_coding((struct usb_cdc_line_coding *)*buf);
		return USBD_REQ_HANDLED;

	case USB_CDC_REQ_GET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;

		if (req->wIndex != 0)
			return USBD_REQ_NOTSUPP;

		usb_serial.get_line_coding((struct usb_cdc_line_coding *)*buf);
		*len = sizeof(struct usb_cdc_line_coding);
		return USBD_REQ_HANDLED;

	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		usb_serial.set_control_line_state(req->wValue);
		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NEXT_CALLBACK;
}

uint16_t usb_cdc_get_config()
{
	return configured;
}

static void cdc_set_config(usbd_device *dev, uint16_t wValue)
{
	configured = wValue;

	// Serial interface
	usb_serial.config();

	usbd_register_control_callback(dev,
								   USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
								   USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
								   cdc_control_request);

	// Send initial serial state.
	// Allows the use of /dev/tty* devices on *BSD/MacOS
	usb_serial.notify_serial_state();
}

void usb_cdc_init()
{
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(USB_PORT_RCC);

#if defined(STM32F0)
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);
#endif

#if defined(STM32F042F6)
	// Remap pins PA11/PA12
	rcc_periph_clock_enable(RCC_SYSCFG_COMP);
	SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
#endif

	// reset USB peripheral
	rcc_periph_reset_pulse(RST_USB);

	// Pull USB D+ low for 80ms to trigger device reenumeration
#if defined(STM32F0)
	gpio_mode_setup(USB_DP_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, USB_DP_PIN);
#elif defined(STM32F1)
	gpio_set_mode(USB_DP_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, USB_DP_PIN);
#endif
	gpio_clear(USB_DP_PORT, USB_DP_PIN);
	delay(80);

	usb_device = usb_conf_init();

	usbd_register_set_config_callback(usb_device, cdc_set_config);

	nvic_set_priority(USB_NVIC_IRQ, IRQ_PRI_USB);
	nvic_enable_irq(USB_NVIC_IRQ);
}

extern "C" void USB_ISR()
{
	usbd_poll(usb_device);
}
