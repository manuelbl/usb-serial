/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB serial implementation
 */

#include "common.h"
#include "hardware.h"
#include "uart.h"
#include "usb_cdc.h"
#include "usb_conf.h"
#include "usb_serial.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/usb/cdc.h>

#define TIMER_FREQ 1000000U // in Hz
#define POLL_FREQ 5000      // in Hz

static void usb_out_cb(usbd_device *dev, uint8_t ep);
static void usb_in_cb(usbd_device *dev, uint8_t ep);

usb_serial_impl usb_serial;

// Called when USB is connected
void usb_serial_impl::config()
{
    is_usb_tx = false;
    is_tx_high_water = false;
    last_serial_state = 0;

    usbd_ep_setup(usb_device, DATA_OUT_1, USB_ENDPOINT_ATTR_BULK, CDCACM_PACKET_SIZE, usb_out_cb);
    usbd_ep_setup(usb_device, DATA_IN_1, USB_ENDPOINT_ATTR_BULK, CDCACM_PACKET_SIZE, usb_in_cb);
    usbd_ep_setup(usb_device, COMM_IN_1, USB_ENDPOINT_ATTR_INTERRUPT, 16, nullptr);

    // configure timer for polling uart RX
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM2, (rcc_apb1_frequency * 2) / TIMER_FREQ - 1);
    timer_set_period(TIM2, TIMER_FREQ / POLL_FREQ - 1);

    nvic_set_priority(NVIC_TIM2_IRQ, IRQ_PRI_UART_TIM);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    timer_enable_counter(TIM2);
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    uart.set_dtr(true); // assert DTR
}

void usb_serial_impl::out_cb(usbd_device *dev, uint8_t ep)
{
    uint8_t packet[CDCACM_PACKET_SIZE] __attribute__((aligned(4)));

    // Retrieve USB data
    uint16_t len = usbd_ep_read_packet(dev, DATA_OUT_1, packet, CDCACM_PACKET_SIZE);
    if (len == 0)
        return;

    // Start transmission via UART
    uart.transmit(packet, len);

    update_nak();
}

// Called when data has arrived via USB
void usb_out_cb(usbd_device *dev, uint8_t ep)
{
    usb_serial.out_cb(dev, ep);
}

// Check for data received via UART
void usb_serial_impl::poll()
{
    bool usb_connected = usb_cdc_get_config() != 0;
    uart.update_leds();
    uart.update_rts(usb_connected);

    if (!usb_connected)
        return;

    uint8_t state = serial_state();
    if (state != last_serial_state) {
        notify_serial_state(state);
    }

    if (is_usb_tx)
        return; // DATA IN endpoint is busy

    uint8_t packet[CDCACM_PACKET_SIZE] __attribute__((aligned(4)));

    // Retrieve UART data
    size_t len = uart.copy_rx_data(packet, CDCACM_PACKET_SIZE);
    if (len == 0)
        return; // no new data

    // Start transmission over USB
    usbd_ep_write_packet(usb_device, DATA_IN_1, packet, len);
    is_usb_tx = true;
}

// Updates the NAK status of DATA_OUT_1
void usb_serial_impl::update_nak()
{
    bool is_high_water = uart.tx_data_avail() < 128; // two more packages
    if (is_high_water != is_tx_high_water) {
        is_tx_high_water = is_high_water;
        usbd_ep_nak_set(usb_device, DATA_OUT_1, is_high_water);
    }
}

// Called when transmission over USB has completed
void usb_serial_impl::in_cb(usbd_device *dev, uint8_t ep)
{
    is_usb_tx = false;
    poll();
}

// Called when transmission over USB has completed
void usb_in_cb(usbd_device *dev, uint8_t ep)
{
    usb_serial.in_cb(dev, ep);
}

void usb_serial_impl::get_line_coding(struct usb_cdc_line_coding *line_coding)
{
    line_coding->dwDTERate = uart.baudrate();
    line_coding->bDataBits = uart.databits();
    line_coding->bCharFormat = (uint8_t)uart.stopbits();
    line_coding->bParityType = (uint8_t)uart.parity();
}

void usb_serial_impl::set_line_coding(struct usb_cdc_line_coding *line_coding)
{
    // sanitize parameters
    if (line_coding->dwDTERate < 733)
        line_coding->dwDTERate = 733;
    if (line_coding->dwDTERate > 3096774) // 48000000 / 15.5 (minimum divider 16 with rounding)
        line_coding->dwDTERate = 3096774;
    if (line_coding->bDataBits != 9)
        line_coding->bDataBits = 8;
    if (line_coding->bCharFormat > 2)
        line_coding->bCharFormat = 0;
    if (line_coding->bParityType > 2)
        line_coding->bParityType = 0;

    uart.set_coding(
        line_coding->dwDTERate,
        line_coding->bDataBits,
        (uart_stopbits)line_coding->bCharFormat,
        (uart_parity)line_coding->bParityType);
    
    uart.update_rts(true);
}

void usb_serial_impl::set_control_line_state(uint16_t state)
{
    uart.set_dtr((state & 1) != 0);
}

uint8_t usb_serial_impl::serial_state()
{
    uint8_t status = 0;
    if (uart.dcd()) status |= 1;
    if (uart.dsr()) status |= 2;
    return status;
}

void usb_serial_impl::notify_serial_state()
{
    notify_serial_state(serial_state());
}

void usb_serial_impl::notify_serial_state(uint8_t state)
{
	char buf[10];
	struct usb_cdc_notification *notif = (struct usb_cdc_notification *)buf;
	notif->bmRequestType = 0xA1;
	notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
	notif->wValue = 0;
	notif->wIndex = 0;
	notif->wLength = 2;
	buf[8] = state;
	buf[9] = 0;
	if (usbd_ep_write_packet(usb_device, COMM_IN_1, buf, 10) == 10)
        last_serial_state = state;
}



// Called every 200µs to check for new UART data
extern "C" void tim2_isr()
{
    timer_clear_flag(TIM2, TIM_SR_UIF);
    usb_serial.poll();
}

#if defined(STM32F0)

// Interrupt handler called when USART DMA has completed an action
extern "C" void dma1_channel4_7_dma2_channel3_5_isr()
{
    uart.on_tx_complete();
    usb_serial.update_nak();
}

#elif defined(STM32F1)

// Interrupt handler called when USART TX DMA has completed an action
extern "C" void dma1_channel7_isr()
{
    if (dma_get_interrupt_flag(USART_DMA, USART_DMA_TX_CHAN, DMA_TCIF))
        uart.on_tx_complete();
}

#endif
