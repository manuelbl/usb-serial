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
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>

#define TX_HOLDBACK_MAX_TIME 3  // max time to hold back data for transmission (in milliseconds)
#define TX_HOLDBACK_MAX_LEN 16  // max number of bytes to hold back data for transmission

static void usb_data_out_cb(usbd_device *dev, uint8_t ep);
static void usb_data_in_cb(usbd_device *dev, uint8_t ep);
static void usb_comm_in_cb(usbd_device *dev, uint8_t ep);

usb_serial_impl usb_serial;

void usb_serial_impl::init()
{
    uart.init();
    usb_cdc_init();
}

// Called when USB is connected
void usb_serial_impl::on_usb_configured()
{
    is_usb_transmitting = false;
    is_tx_high_water = false;
    last_serial_state = 0;
    tx_timestamp = millis() - 100;
    pending_interrupt = 0;

    // register callbacks
    usbd_ep_setup(usb_device, DATA_OUT_1, USB_ENDPOINT_ATTR_BULK, CDCACM_PACKET_SIZE, usb_data_out_cb);
    usbd_ep_setup(usb_device, DATA_IN_1, USB_ENDPOINT_ATTR_BULK, CDCACM_PACKET_SIZE, usb_data_in_cb);
    usbd_ep_setup(usb_device, COMM_IN_1, USB_ENDPOINT_ATTR_INTERRUPT, 16, usb_comm_in_cb);

    // assert DTR
    uart.enable();
    uart.set_dtr(true);
}

void usb_serial_impl::on_usb_data_received(usbd_device *dev)
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
void usb_data_out_cb(usbd_device *dev, __attribute__((unused)) uint8_t ep)
{
    usb_serial.on_usb_data_received(dev);
}

bool usb_serial_impl::is_connected()
{
    return usb_cdc_is_connected();
}

// Check for data received via UART
void usb_serial_impl::poll()
{
    usb_cdc_poll();
    
    uart.poll();

    if (!usb_cdc_is_connected())
        return;

    update_nak();

    // Check for RX buffer overrun
    if (uart.has_rx_overrun_occurred()) {
        on_interrupt_occurred(usb_serial_interrupt::data_overrun);
        return;
    }

    uint16_t state = serial_state();
    if (state != last_serial_state)
        notify_serial_state(state);

    if (is_usb_transmitting)
        return; // DATA IN endpoint is busy

    // In order to prevent the USB line from being flooded with packets
    // to transmit a single byte, data is held back until a certain time
    // has expired or a certain number of bytes has been accumulated.
    // After a pause with no transmission, the next byte (or chunk of bytes)
    // is immediately transmitted.
    size_t len = uart.rx_data_len();
    if (len < TX_HOLDBACK_MAX_LEN && !has_expired(tx_timestamp + TX_HOLDBACK_MAX_TIME))
        return; // wait for more data to arrive

    tx_timestamp = millis();

    uint8_t packet[CDCACM_PACKET_SIZE] __attribute__((aligned(4)));

    // Retrieve UART data
    len = uart.copy_rx_data(packet, CDCACM_PACKET_SIZE);
    if (len == 0)
        return; // no new data

    // Start transmission over USB
    usbd_ep_write_packet(usb_device, DATA_IN_1, packet, len);
    is_usb_transmitting = true;
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
void usb_serial_impl::on_usb_data_transmitted()
{
    is_usb_transmitting = false;
}

// Called when transmission over USB has completed
void usb_data_in_cb(__attribute__((unused)) usbd_device *dev, __attribute__((unused)) uint8_t ep)
{
    usb_serial.on_usb_data_transmitted();
}

void usb_serial_impl::get_line_coding(struct usb_cdc_line_coding *line_coding)
{
    line_coding->dwDTERate = uart.baudrate();
    line_coding->bDataBits = uart.databits();
    line_coding->bCharFormat = (uint8_t)uart.stopbits();
    line_coding->bParityType = (uint8_t)uart.parity();
}

bool usb_serial_impl::set_line_coding(struct usb_cdc_line_coding *line_coding)
{
    // sanitize parameters
    // 3096774 = 48000000 / 15.5 (minimum divider 16 with rounding)
    if (line_coding->dwDTERate < 733)
        return false;
    if (line_coding->dwDTERate > 3096774)
        return false;

    if (line_coding->bCharFormat > 2)
        return false;
    if (line_coding->bParityType > 2)
        return false;

    // supported combinations (data + parity bits): 7 + 1, 8 + 0, 8 + 1
    if (line_coding->bParityType == 0) {
        if (line_coding->bDataBits != 8)
            return false;
    } else {
        if (line_coding->bDataBits < 7 || line_coding->bDataBits > 8)
            return false;
    }

    uart.set_coding(
        line_coding->dwDTERate,
        line_coding->bDataBits,
        (uart_stopbits)line_coding->bCharFormat,
        (uart_parity)line_coding->bParityType);
    
    return true;
}

void usb_serial_impl::set_control_line_state(uint16_t state)
{
    uart.set_dtr((state & 1) != 0);
}

uint16_t usb_serial_impl::serial_state()
{
    uint16_t status = (uint16_t)pending_interrupt;
    if (uart.dcd()) status |= 1;
    if (uart.dsr()) status |= 2;
    return status;
}

void usb_serial_impl::send_serial_state()
{
    notify_serial_state(serial_state());
}

void usb_serial_impl::notify_serial_state(uint16_t state)
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
	if (usbd_ep_write_packet(usb_device, COMM_IN_1, buf, 10) == 10) {
        last_serial_state = state & 0x3;
        pending_interrupt = 0;
    }
}

void usb_serial_impl::on_interrupt_occurred(usb_serial_interrupt interrupt)
{
    pending_interrupt |= (uint16_t)interrupt;
    send_serial_state();
}

void usb_serial_impl::on_usb_ctrl_completed()
{
    uint16_t state = serial_state();
    if (state != last_serial_state)
        notify_serial_state(state);
}

// Called when control data has been received or transmitted via USB
void usb_comm_in_cb(__attribute__((unused)) usbd_device *dev, __attribute__((unused)) uint8_t ep)
{
    usb_serial.on_usb_ctrl_completed();
}
