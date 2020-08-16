/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Common functions
 */

#include "common.h"
#include <libopencm3/stm32/rcc.h>

static volatile uint32_t millis_count;

uint32_t millis()
{
	return millis_count;
}

void delay(uint32_t ms)
{
	int32_t target_time = millis_count + ms;
	while (target_time - (int32_t)millis_count > 0)
		;
}

bool has_expired(uint32_t timeout)
{
    return (int32_t)timeout - (int32_t)millis_count <= 0;
}

void common_init()
{
	// Initialize SysTick

#if defined(STM32F0)

	rcc_clock_setup_in_hsi_out_48mhz();

	// Interrupt every 1ms
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000 - 1);

#elif defined(STM32F1)

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	// Interrupt every 1ms
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	systick_set_reload(rcc_ahb_frequency / 8 / 1000 - 1);
	
#endif

	// Enable and start
	systick_interrupt_enable();
	systick_counter_enable();
}

// System tick timer interrupt handler
extern "C" void sys_tick_handler()
{
	millis_count++;
}

