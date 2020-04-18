/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * UART implementation
 */

#include "common.h"
#include "hardware.h"
#include "uart.h"
#include "usb_dev.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <algorithm>
#include <string.h>

uart_impl uart;

void uart_impl::init()
{
    tx_state = uart_state::ready;
    tx_buf_head = tx_buf_tail = 0;
    tx_size = 0;
    rx_buf_tail = 0;
    rx_led_timeout_active = tx_led_timeout_active = false;
    rx_led_head = 0;

    // Enable USART interface clock
    rcc_periph_clock_enable(USART_RCC);

    // Enable TX, RX pin clock
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure RX/TXpins
    gpio_set(USART_PORT, USART_TX_GPIO);
#if defined(STM32F0)
    gpio_mode_setup(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_TX_GPIO | USART_RX_GPIO);
    gpio_set_af(USART_PORT, GPIO_AF1, USART_TX_GPIO | USART_RX_GPIO);
#elif defined(STM32F1)
    gpio_set_mode(USART_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, USART_TX_GPIO);
    gpio_set_mode(USART_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, USART_RX_GPIO);
#endif

    // configure RTS/CTS
    rcc_periph_clock_enable(RTS_PORT_RCC);
    rcc_periph_clock_enable(CTS_PORT_RCC);

    gpio_set(RTS_PORT, RTS_PIN); // initial state: not asserted

#if defined(STM32F0)
    gpio_mode_setup(RTS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RTS_PIN);
    gpio_mode_setup(CTS_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, CTS_PIN);
    gpio_set_af(CTS_PORT, GPIO_AF1, CTS_PIN);
#elif defined(STM32F1)
    gpio_set_mode(RTS_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, RTS_PIN);
    gpio_set_mode(CTS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, CTS_PIN);
	gpio_clear(CTS_PORT, CTS_PIN); // pull down
#endif

    // configure TX DMA
    rcc_periph_clock_enable(USART_DMA_RCC);
    dma_channel_reset(USART_DMA, USART_DMA_TX_CHAN);
    dma_set_peripheral_address(USART_DMA, USART_DMA_TX_CHAN, (uint32_t)&USART_TX_DATA_REG);
    dma_set_read_from_memory(USART_DMA, USART_DMA_TX_CHAN);
    dma_enable_memory_increment_mode(USART_DMA, USART_DMA_TX_CHAN);
    dma_set_memory_size(USART_DMA, USART_DMA_TX_CHAN, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(USART_DMA, USART_DMA_TX_CHAN, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(USART_DMA, USART_DMA_TX_CHAN, DMA_CCR_PL_MEDIUM);
    dma_enable_transfer_complete_interrupt(USART_DMA, USART_DMA_TX_CHAN);

    // configure RX DMA (as circular buffer)
    dma_channel_reset(USART_DMA, USART_DMA_RX_CHAN);
    dma_set_peripheral_address(USART_DMA, USART_DMA_RX_CHAN, (uint32_t)&USART_RX_DATA_REG);
    dma_set_read_from_peripheral(USART_DMA, USART_DMA_RX_CHAN);
    dma_enable_memory_increment_mode(USART_DMA, USART_DMA_RX_CHAN);
    dma_enable_circular_mode(USART_DMA, USART_DMA_RX_CHAN);
    dma_set_memory_size(USART_DMA, USART_DMA_RX_CHAN, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(USART_DMA, USART_DMA_RX_CHAN, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(USART_DMA, USART_DMA_RX_CHAN, DMA_CCR_PL_MEDIUM);
    dma_set_memory_address(USART_DMA, USART_DMA_RX_CHAN, (uint32_t)rx_buf);
    dma_set_number_of_data(USART_DMA, USART_DMA_RX_CHAN, UART_RX_BUF_LEN);

    dma_enable_channel(USART_DMA, USART_DMA_RX_CHAN);
    usart_enable_rx_dma(USART);

    // enable DMA interrupt (notifying about a completed transmission)
#if defined(STM32F0)
    nvic_set_priority(USART_DMA_NVIC_IRQ, IRQ_PRI_UART_DMA);
    nvic_enable_irq(USART_DMA_NVIC_IRQ);
#elif defined(STM32F1)
    nvic_set_priority(USART_DMA_TX_NVIC_IRQ, IRQ_PRI_UART_DMA);
    nvic_enable_irq(USART_DMA_TX_NVIC_IRQ);
#endif

    // configure baud rate etc.
    set_coding(9600, 8, uart_stopbits::bits_1_0, uart_parity::no_parity);
    usart_set_mode(USART, USART_MODE_TX_RX);
    usart_set_flow_control(USART, USART_FLOWCONTROL_CTS);
    usart_enable(USART);

    // configure RX/TX LEDs
    rcc_periph_clock_enable(LED_RX_PORT_RCC);
    rcc_periph_clock_enable(LED_TX_PORT_RCC);

    gpio_clear(LED_RX_PORT, LED_RX_PIN);
    gpio_clear(LED_TX_PORT, LED_TX_PIN);

#if defined(STM32F0)
    gpio_mode_setup(LED_RX_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RX_PIN);
    gpio_mode_setup(LED_TX_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_TX_PIN);
#elif defined(STM32F1)
    gpio_set_mode(LED_RX_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_RX_PIN);
    gpio_set_mode(LED_TX_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_TX_PIN);
#endif

    // configure DTR/DSR/DCD
    rcc_periph_clock_enable(DTR_PORT_RCC);
    rcc_periph_clock_enable(DSR_PORT_RCC);
    rcc_periph_clock_enable(DCD_PORT_RCC);

    gpio_set(DTR_PORT, DTR_PIN); // initial state: not asserted

#if defined(STM32F0)
    gpio_mode_setup(DTR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DTR_PIN);
    gpio_mode_setup(DSR_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, DSR_PIN);
    gpio_mode_setup(DCD_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, DCD_PIN);
#elif defined(STM32F1)
    gpio_set_mode(DTR_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DTR_PIN);
    gpio_set_mode(DSR_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, DSR_PIN);
	gpio_clear(DSR_PORT, DSR_PIN); // pull down
    gpio_set_mode(DCD_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, DCD_PIN);
	gpio_clear(DCD_PORT, DCD_PIN); // pull down
#endif
}

void uart_impl::transmit(const uint8_t *data, size_t len)
{
    int buf_tail = tx_buf_tail;
    int buf_head = tx_buf_head;

    size_t avail_chunk_size;
    if (buf_head < buf_tail)
        avail_chunk_size = buf_tail - buf_head - 1;
    else if (buf_tail != 0)
        avail_chunk_size = UART_TX_BUF_LEN - buf_head;
    else
        avail_chunk_size = UART_TX_BUF_LEN - 1 - buf_head;

    if (avail_chunk_size == 0)
        return; // buffer full - discard data

    // Copy data to transmit buffer
    size_t size = std::min(len, avail_chunk_size);
    memcpy(tx_buf + buf_head, data, size);
    buf_head += size;
    if (buf_head >= UART_TX_BUF_LEN)
        buf_head = 0;
    tx_buf_head = buf_head;

    // start transmission
    start_transmit();

    // Use second transmit in case of remaining data
    // (usually because of wrap around at the end of the buffer)
    if (size < len)
        transmit(data + size, len - size);
}


void uart_impl::transmit_usb_rx(uint8_t addr)
{
    if (usb_dev_get_rx_count(addr) > tx_data_avail())
        return; // insufficent space in ring buffer

    // copy data to ring buffer
    tx_buf_head = usb_dev_copy_from_pm(addr, tx_buf, UART_TX_BUF_LEN, tx_buf_head);

    // start transmission
    start_transmit();
}


void uart_impl::start_transmit()
{
    int start_pos;

    {
        irq_guard guard;

        if (tx_state != uart_state::ready || tx_buf_head == tx_buf_tail)
            return; // UART busy or queue empty

        // Determine TX chunk size
        start_pos = tx_buf_tail;
        int end_pos = tx_buf_head;
        if (end_pos <= start_pos)
            end_pos = UART_TX_BUF_LEN;
        tx_size = end_pos - start_pos;
        if (tx_size > 32)
            tx_size = 32; // no more than 32 bytes to free up space soon
        tx_state = uart_state::transmitting;
    }

    // set transmit chunk
    dma_set_memory_address(USART_DMA, USART_DMA_TX_CHAN, (uint32_t)(tx_buf + start_pos));
    dma_set_number_of_data(USART_DMA, USART_DMA_TX_CHAN, tx_size);

    // start transmission
    dma_enable_channel(USART_DMA, USART_DMA_TX_CHAN);
    usart_enable_tx_dma(USART);

    // turn on TX LED
    tx_led_timeout_active = false;
    gpio_set(LED_TX_PORT, LED_TX_PIN);
}

void uart_impl::on_tx_complete()
{
    if (!dma_get_interrupt_flag(USART_DMA, USART_DMA_TX_CHAN, DMA_TCIF))
        return;

    {
        irq_guard guard;

        // Update TX buffer
        int buf_tail = tx_buf_tail + tx_size;
        if (buf_tail >= UART_TX_BUF_LEN)
            buf_tail = 0;
        tx_buf_tail = buf_tail;
        tx_size = 0;
        tx_state = uart_state::ready;
    }

    // Disable DMA    
    dma_disable_channel(USART_DMA, USART_DMA_TX_CHAN);
    dma_clear_interrupt_flags(USART_DMA, USART_DMA_TX_CHAN, DMA_TCIF);

    // Turn off LED in 100ms
    tx_led_timeout_active = true;
    tx_led_off_timeout = millis() + 100;

    // Check for next transmission
    start_transmit();
}

size_t uart_impl::copy_rx_data(uint8_t *data, size_t len)
{
    int buf_head = UART_RX_BUF_LEN - dma_get_number_of_data(USART_DMA, USART_DMA_RX_CHAN);
    if (buf_head == rx_buf_tail)
        return 0; // no new data

    int n1 = 0;
    int n2 = 0;

    if (rx_buf_tail > buf_head)
    {
        // chunk between tail and end of buffer
        n1 = std::min((int)len, UART_RX_BUF_LEN - rx_buf_tail);
        memcpy(data, rx_buf + rx_buf_tail, n1);
        rx_buf_tail += n1;
        if (rx_buf_tail >= UART_RX_BUF_LEN)
            rx_buf_tail = 0;
        data += n1;
        len -= n1;
    }

    if (len == 0)
        return n1;

    if (rx_buf_tail < buf_head)
    {
        // chunk between tail and head (no wrap around)
        n2 = std::min((int)len, buf_head - rx_buf_tail);
        memcpy(data, rx_buf + rx_buf_tail, n2);
        rx_buf_tail += n2;
    }

    return n1 + n2;
}

size_t uart_impl::rx_data_len()
{
    int buf_head = UART_RX_BUF_LEN - dma_get_number_of_data(USART_DMA, USART_DMA_RX_CHAN);
    if (buf_head >= rx_buf_tail)
        return buf_head - rx_buf_tail;

    return UART_RX_BUF_LEN - rx_buf_tail + buf_head;
}

size_t uart_impl::tx_data_avail() {
    int head = tx_buf_head;
    int tail = tx_buf_tail;
    if (head >= tail)
        return UART_TX_BUF_LEN - (head - tail) - 1;
    else
        return tail - head - 1;    
}


void uart_impl::set_dtr(bool asserted)
{
    if (asserted)
        gpio_clear(DTR_PORT, DTR_PIN); // active low
    else
        gpio_set(DTR_PORT, DTR_PIN);    
}

bool uart_impl::dsr()
{
    return gpio_get(DSR_PORT, DSR_PIN) == 0;
}

bool uart_impl::dcd()
{
    return gpio_get(DCD_PORT, DCD_PIN) == 0;
}

void uart_impl::update_leds()
{
    // check for TX LED timeout
    if (tx_led_timeout_active) {
        if (has_expired(tx_led_off_timeout)) {
            gpio_clear(LED_TX_PORT, LED_TX_PIN);
            tx_led_timeout_active = false;
        }
    }

    // check for RX LED timeout
    if (rx_led_timeout_active) {
        if (has_expired(rx_led_off_timeout)) {
            gpio_clear(LED_RX_PORT, LED_RX_PIN);
            rx_led_timeout_active = false;
        }
    }

    // check for new received data (relevant for LED only)
    int buf_head = UART_RX_BUF_LEN - dma_get_number_of_data(USART_DMA, USART_DMA_RX_CHAN);
    if (buf_head != rx_led_head) {
        // turn on RX LED and set timeout of 100ms
        gpio_set(LED_RX_PORT, LED_RX_PIN);
        rx_led_timeout_active = true;
        rx_led_off_timeout = millis() + 100;
        rx_led_head = buf_head;
    }
}

void uart_impl::update_rts(bool ready)
{
    bool asserted = ready && (int)rx_data_len() < rx_high_water_mark;
    if (asserted)
        gpio_clear(RTS_PORT, RTS_PIN);
    else
        gpio_set(RTS_PORT, RTS_PIN);
}

static const uint32_t stopbits_enum_to_uint32[] = {
    USART_STOPBITS_1,
    USART_STOPBITS_1_5,
    USART_STOPBITS_2,
};

static const uint32_t parity_enum_to_uint32[] = {
    USART_PARITY_NONE,
    USART_PARITY_ODD,
    USART_PARITY_EVEN,
};

void uart_impl::set_coding(int baudrate, int databits, uart_stopbits stopbits, uart_parity parity)
{
    _baudrate = baudrate;
    _databits = databits;
    _stopbits = stopbits;
    _parity = parity;

    // TODO: disable USART for change of baud rate
    usart_set_baudrate(USART, _baudrate);
    usart_set_databits(USART, _databits);
    usart_set_stopbits(USART, stopbits_enum_to_uint32[(int)_stopbits]);
    usart_set_parity(USART, parity_enum_to_uint32[(int)_parity]);

    // High water mark is buffer size - 5ms worth of data
    rx_high_water_mark = std::max(UART_RX_BUF_LEN - baudrate / 2000, 0);
}
