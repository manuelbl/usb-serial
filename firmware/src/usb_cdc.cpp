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
#if defined(STM32F0)
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/syscfg.h>
#endif
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "qsb_device.h"
#include <string.h>

qsb_device *usb_device;

static uint16_t configured;

// Process ACM requests on control endpoint
static enum qsb_request_return_code cdc_control_request(
	__attribute__((unused)) qsb_device *dev,
	qsb_setup_data *req, uint8_t **buf, uint16_t *len,
	__attribute__((unused)) qsb_dev_control_completion_callback_fn *complete)
{
	switch (req->bRequest)
	{
	case QSB_PSTN_REQ_SET_LINE_CODING:
		if (*len < sizeof(qsb_pstn_line_coding))
			return QSB_REQ_NOTSUPP;

		if (req->wIndex != 0)
			return QSB_REQ_NOTSUPP;

		return usb_serial.set_line_coding((qsb_pstn_line_coding *)*buf) ? QSB_REQ_HANDLED : QSB_REQ_NOTSUPP;
		

	case QSB_PSTN_REQ_GET_LINE_CODING:
		if (*len < sizeof(qsb_pstn_line_coding))
			return QSB_REQ_NOTSUPP;

		if (req->wIndex != 0)
			return QSB_REQ_NOTSUPP;

		usb_serial.get_line_coding((qsb_pstn_line_coding *)*buf);
		*len = sizeof(qsb_pstn_line_coding);
		return QSB_REQ_HANDLED;

	case QSB_PSTN_REQ_SET_CONTROL_LINE_STATE:
		usb_serial.set_control_line_state(req->wValue);
		return QSB_REQ_HANDLED;
	}
	return QSB_REQ_NEXT_HANDLER;
}

bool usb_cdc_is_connected()
{
	return configured != 0;
}

static void cdc_set_config(qsb_device *dev, uint16_t wValue)
{
	configured = wValue;

	qsb_dev_register_control_callback(dev,
								   QSB_REQ_TYPE_CLASS     | QSB_REQ_TYPE_INTERFACE,
								   QSB_REQ_TYPE_TYPE_MASK | QSB_REQ_TYPE_RECIPIENT_MASK,
								   cdc_control_request);

	// Serial interface
	usb_serial.on_usb_configured();

	// Send initial serial state.
	// Allows the use of /dev/tty* devices on macOS and BSD systems
	usb_serial.send_serial_state();
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

	// create USB device
	usb_device = usb_conf_init();

	// Set callback for config calls
	qsb_dev_register_set_config_callback(usb_device, cdc_set_config);
}

void usb_cdc_poll()
{
	qsb_dev_poll(usb_device);
}
