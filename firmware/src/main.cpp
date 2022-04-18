/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Main program
 */

#include "common.h"
#include "hardware.h"
#include "usb_conf.h"
#include "usb_serial.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/desig.h>


static void gpio_setup()
{
	// configure power LED
	rcc_periph_clock_enable(LED_POWER_PORT_RCC);

#if defined(STM32F0)
	gpio_mode_setup(LED_POWER_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_POWER_PIN);
#elif defined(STM32F1)
	gpio_set_mode(LED_POWER_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_POWER_PIN);
#endif

#if defined(LED_POWER_REVERSED)
	gpio_clear(LED_POWER_PORT, LED_POWER_PIN);
#else
	gpio_set(LED_POWER_PORT, LED_POWER_PIN);
#endif
}

int main()
{
	common_init();
	gpio_setup();
	qsb_serial_num_init();
	usb_serial.init();

	bool connected = false;
	uint32_t next_led_toggle = 0;

	while (1)
	{
		usb_serial.poll();

		if (!connected)
		{
			if (usb_serial.is_connected())
			{
				// USB has just been connected: turn on power LED for good
#if defined(LED_POWER_REVERSED)
				gpio_clear(LED_POWER_PORT, LED_POWER_PIN);
#else
				gpio_set(LED_POWER_PORT, LED_POWER_PIN);
#endif
				connected = true;
			}
			else if (has_expired(next_led_toggle))
			{
				// USB not yet connected: blink power LED quickly
				gpio_toggle(LED_POWER_PORT, LED_POWER_PIN);
				next_led_toggle = millis() + 150;
			}
		}
	}

	return 0;
}
