/*
 * USB Serial - Throttler firmware
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Test fixture for testing hardware flow control (CTS/RTS).
 * 
 * This firmware passes data received on UART1 RX to UART2 TX and vice versa 
 * (UART2 RX to UART1 TX). At the interface, it works with 115,200 bps.
 * Internally, it throttles the speed to 2 bytes/ms (about about 20,000 bps).
 */

#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <algorithm>

#define BUF_SIZE 512
#define MAX_CAPACITY 16
#define SPEED 2 // bytes per ms

static volatile bool tick_occurred;

static int32_t capacity_ch_a;
static int32_t capacity_ch_b;

static int32_t size_ch_a;
static int32_t head_ch_a;
static int32_t tail_ch_a;
static int32_t size_ch_b;
static int32_t head_ch_b;
static int32_t tail_ch_b;

uint8_t buffer_ch_a[BUF_SIZE];
uint8_t buffer_ch_b[BUF_SIZE];

extern "C" void sys_tick_handler()
{
	tick_occurred = true;
}

static void clock_setup()
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	// Interrupt every 1ms
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	systick_set_reload(rcc_ahb_frequency / 8 / 1000 - 1);

	// Enable and start
	systick_interrupt_enable();
	systick_counter_enable();
}

static void uart_setup()
{
	// Enable USART interface clock
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_USART2);

	// Enable pin clock
	rcc_periph_clock_enable(RCC_GPIOA);

	// Enable AFIO clock for remapping
	rcc_periph_clock_enable(RCC_AFIO);

	// Remap CAN1 (conflict with USART1 RTS, see errata)
	AFIO_MAPR |= AFIO_MAPR_CAN1_REMAP_PORTB;

	// Configure pins for USART1
	gpio_set(GPIOA, GPIO9);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9); // TX
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);				  // RX
	gpio_clear(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO12); // RTS
	gpio_clear(GPIOA, GPIO11);															   // CTS pull down
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO11);			   // CTS

	// Configure USART1
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_RTS_CTS);
	usart_enable(USART1);

	// Configure pins for USART2
	gpio_set(GPIOA, GPIO2);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO2); // TX
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO3);					  // RX
	gpio_clear(GPIOA, GPIO1);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO1); // RTS
	gpio_clear(GPIOA, GPIO0);															  // CTS pull down
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);			  // CTS

	// Configure USART2
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_RTS_CTS);
	usart_enable(USART2);
}

int main()
{
	clock_setup();
	uart_setup();

	while (1)
	{
		// --- Channel A: USART 1 to USART 2 ---

		if (size_ch_a > 0 && (USART_SR(USART2) & USART_SR_TXE) != 0)
		{
			// send byte
			USART_DR(USART2) = buffer_ch_a[tail_ch_a];
			tail_ch_a++;
			if (tail_ch_a >= BUF_SIZE)
				tail_ch_a = 0;
			size_ch_a--;
		}

		if (capacity_ch_a > 0 && size_ch_a < BUF_SIZE && (USART_SR(USART1) & USART_SR_RXNE) != 0)
		{
			// receive byte
			capacity_ch_a--;
			buffer_ch_a[head_ch_a] = (uint8_t)USART_DR(USART1);
			head_ch_a++;
			if (head_ch_a >= BUF_SIZE)
				head_ch_a = 0;
			size_ch_a++;
		}

		// --- Channel B: USART 2 to USART 1 ---

		if (size_ch_b > 0 && (USART_SR(USART1) & USART_SR_TXE) != 0)
		{
			// send byte
			USART_DR(USART1) = buffer_ch_b[tail_ch_b];
			tail_ch_b++;
			if (tail_ch_b >= BUF_SIZE)
				tail_ch_b = 0;
			size_ch_b--;
		}

		if (capacity_ch_b > 0 && size_ch_b < BUF_SIZE && (USART_SR(USART2) & USART_SR_RXNE) != 0)
		{
			// receive byte
			capacity_ch_b--;
			buffer_ch_b[head_ch_b] = (uint8_t)USART_DR(USART2);
			head_ch_b++;
			if (head_ch_b >= BUF_SIZE)
				head_ch_b = 0;
			size_ch_b++;
		}

		// Increase capacity on each systick

		if (tick_occurred)
		{
			capacity_ch_a = std::min(capacity_ch_a + SPEED, (int32_t)MAX_CAPACITY);
			capacity_ch_b = std::min(capacity_ch_b + SPEED, (int32_t)MAX_CAPACITY);
			tick_occurred = false;
		}
	}

	return 0;
}
