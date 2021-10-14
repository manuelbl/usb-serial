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


static void get_unique_id_as_string(char* serial_no);
static void format_base32(uint32_t value, char* buf, uint8_t len);


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

void init_serial_no()
{
	// set serial number in USB descriptor
	char serial_no[USB_SERIAL_NUM_LENGTH];
	get_unique_id_as_string(serial_no);
	usb_set_serial_number(serial_no);
}

int main()
{
	common_init();
	gpio_setup();
	init_serial_no();
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

/**
 * @brief Gets a repeatable, more or less unique ID string
 * 
 * The ID string is derived from the data in the unique device ID
 * register (device electronic signature).
 * 
 * The resulting ID string is 10 characters long.
 * 
 * @param serial_no buffer receiving the ID string (at least 11 bytes long)
 */
void get_unique_id_as_string(char* serial_no)
{
    uint32_t id0 = DESIG_UNIQUE_ID0;
    uint32_t id1 = DESIG_UNIQUE_ID1;
    uint32_t id2 = DESIG_UNIQUE_ID2;

    id0 += id2;
	id1 = (id1 >> 2) | ((id0 & 0x03) << 30);

    format_base32(id0, serial_no, 6);
    format_base32(id1, serial_no + 6, 4);
	serial_no[10] = 0;
}

void format_base32(uint32_t value, char* buf, uint8_t len)
{
	const char base32_digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    for (int i = 0; i < len; i++) {
        uint8_t digit = value >> 27;
        buf[i] = base32_digits[digit];
        value = value << 5;
    }
}
