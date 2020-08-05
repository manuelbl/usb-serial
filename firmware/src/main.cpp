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
#include "uart.h"
#include "usb_conf.h"
#include "usb_cdc.h"
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

	gpio_set(LED_POWER_PORT, LED_POWER_PIN);
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
	uart.init();
	init_serial_no();
	usb_cdc_init();

	// This code only takes care of the power LED.
	// All the magic happens in interrupt handlers.
		
	while (1)
	{
		if (is_usb_connected()) {
			// USB connected: turn on power LED
			gpio_set(LED_POWER_PORT, LED_POWER_PIN);
			delay(100);
		} else {
			// USB not connected yet: blink LED
			gpio_toggle(LED_POWER_PORT, LED_POWER_PIN);
			delay(150);
			gpio_toggle(LED_POWER_PORT, LED_POWER_PIN);
			delay(150);
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
