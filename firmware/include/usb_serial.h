/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB serial implementation
 */

#pragma once

#include "qsb_device.h"
#include "qsb_cdc.h"


/**
 * @brief Interrupts the host is notified about
 */
enum class usb_serial_interrupt : uint16_t
{
    /// Data overrun (received data has been discarded)
    data_overrun = 64,
    /// Parity error
    parity_error = 32
};


/**
 * @brief USB Serial implementation
 * 
 * Implements a USB CDC PSTN class device.
 */
class usb_serial_impl
{
public:
    /// Initializes the UART and the USB connection
    void init();

    /// Called when the USB interface is configured
    void on_usb_configured();

    /**
     * @brief Gets the line coding information from the global UART instance
     * 
     * Used to implement a GET_LINE_CODING request.
     * 
     * @param line_coding line coding information to fill in, in format defined by USB CDC PSTN standard
     */
    void get_line_coding(qsb_pstn_line_coding *line_coding);

    /**
     * @brief Sets the line coding information of the global UART instance
     * 
     * This member function is called to process a SET_LINE_CODING request.
     * 
     * @param line_coding line coding information, in format defined by USB CDC PSTN standard
     * @return `true` if successful, `false` if line coding parameters are not supported
     */
    bool set_line_coding(qsb_pstn_line_coding *line_coding);

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
    uint16_t serial_state();

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
    void on_usb_data_received(qsb_device *dev);

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

    /**
     * @brief Notifies the host that an interrupt has occurred.
     * 
     * @param interrupt interrupt
     */
    void on_interrupt_occurred(usb_serial_interrupt interrupt);

    /**
     * @brief Called when controlled data has been transmitted or received via USB.
     * 
     * Checks if further notifications are pending.
     */
    void on_usb_ctrl_completed();

    /**
     * Indicates if the USB CDC connection if configured.
     * 
     * @return `true` if connected
     */
    bool is_connected();

private:
    void notify_serial_state(uint16_t state);

    // indicates if zero-length packet is needed as previously transmitted packet was equal to maximum packet size
    bool needs_zlp;

    // Indicates if the UART transmit buffer is almost full
    // (used to set USB NAK to prevent receiving more data)
    bool is_tx_high_water;

    // Last serial state sent to host
    uint16_t last_serial_state;

    // Timestamp of last data transmitted via USB (in milliseconds)
    uint32_t tx_timestamp;

    // Interrupt the host needs to be notified about
    uint16_t pending_interrupt;
};

/// Global USB Serial instance
extern usb_serial_impl usb_serial;
