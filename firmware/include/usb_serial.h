/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB serial implementation
 */

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "usb_cdc.h"
#include <libopencm3/usb/cdc.h>

/**
 * @brief USB Serial implementation
 * 
 * Implements a USB CDC PSTN class device.
 */
class usb_serial_impl
{
public:
    /// Called when the USB interface is configured
    void on_usb_configured();

    /**
     * @brief Gets the line coding information from the global UART instance
     * 
     * Used to implement a GET_LINE_CODING request.
     * 
     * @param line_coding line coding information to fill in, in format defined by USB CDC PSTN standard
     */
    void get_line_coding(struct usb_cdc_line_coding *line_coding);

    /**
     * @brief Sets the line coding information of the global UART instance
     * 
     * This member function is called to process a SET_LINE_CODING request.
     * 
     * @param line_coding line coding information, in format defined by USB CDC PSTN standard
     */
    void set_line_coding(struct usb_cdc_line_coding *line_coding);

    /**
     * @brief Sets the control line state of the global UART instance.
     * 
     * This member function is called to process a SET_CONTROL_LINE_STATE request.
     * It sets the DTR output signal.
     * 
     * @param state control line state, in format defined by USB CDC PSTN standard
     */
    void set_control_line_state(uint16_t state);

    /**
     * @brief Gets the serial state from global UART instance.
     * 
     * The serial state consists of the DCD and DSR input signal
     * as well as error conditions.
     * 
     * @return serial state, in format defined by USB CDC PSTN standard
     */
    uint8_t serial_state();

    /**
     * @brief Sends the serial state to the USB host.
     * 
     * The serial state is sent using a SERIAL_STATE notification.
     */
    void send_serial_state();

    /**
     * @brief Called when data has been received via USB.
     * 
     * Transmits the received data via the UART.
     * 
     * @param dev USB device
     */
    void on_usb_data_received(usbd_device *dev);

    /**
     * @brief Called when data has been transmitted via USB.
     * 
     * Checks if more data has been received via UART and
     * is ready to be transmitted.
     */
    void on_usb_data_transmitted();

    /**
     * @brief Polls for new data received via UART.
     * 
     * Additionally checks for a changed serial state that
     * need to be sent to the host and updates LEDs and RTS output signal.
     */
    void poll();

    /**
     * @brief Updates the USB NAK state
     * 
     * NAK is used for flow control on the USB connection, in particular to
     * indicate that the device currently cannot receive additional data.
     */
    void update_nak();

    // Tick counter is incremented every 0.2ms
    uint32_t tick;

private:
    void notify_serial_state(uint8_t state);

    // indicates if data is being transmitted over USB
    bool is_usb_transmitting;

    // Indicates if the UART transmit buffer is almost full
    // (used to set USB NAK to prevent receiving more data)
    bool is_tx_high_water;

    // Last serial state sent to host
    uint8_t last_serial_state;

    // Timestamp (tick value) of last data transmitted via USB
    uint32_t tx_timestamp;
};

/// Global USB Serial instance
extern usb_serial_impl usb_serial;

#endif
