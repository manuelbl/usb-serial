//
//  USB Serial
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Serial port class (for macOS).
//

#pragma once

#include <exception>
#include <string>


/**
 * Serial port.
 *
 * Lightweight class for working with serial port.
 *
 * Opening and closing the serial port is not tied to the life-cycle of the
 * instances. It needs to be done explicilty. Instances can be copied.
 * However, only one instance may closed.
 *
 * It is safe to transmit and receive data from two separate threads.
 * It is not safe to transmit from multiple threads or to receive from
 * multiple threads without further synchronization.
 */
class serial_port {
public:
    /**
     * Create a new serial port instance.
     *
     * The serial port is in closed state.
     */
    serial_port();
    
    /**
     * Open the specified serial port.
     *
     * @param path serial port path name, like `/dev/cu.usb12345`
     * @param bit_rate bit rate (in baud or bits/s)
     * @param data_bits number of data bits (7 or 8)
     * @param with_parity whehter to use an additional parity bit
     */
    void open(const char* path, int bit_rate, int data_bits = 8, bool with_parity = false);
    
    /**
     * Close this serial port.
     */
    void close();
    
    /**
     * Transmit data on this serial port.
     *
     * @param data data to transmit
     * @param data_len length of data, in bytes
     */
    void transmit(const uint8_t* data, int data_len);
    
    /**
     * Receive data from the serial port.
     *
     * @param data buffer to receive data
     * @param data_len length of buffer, in bytes
     * @return number of bytes received
     */
    int receive(uint8_t* data, int data_len);
    
    /**
     * Drains any pending data.
     */
    void drain();
    
private:
    int _fd;
    
};


/**
 * Serial port error.
 */
class serial_error : public std::exception {
public:
    /**
     * Create a new exception instance.
     *
     * If a error number is provided, it is interpreted as a system error number and
     * the correspoding system message is looked up. The provided message and
     * the system message are then combined into the final message.
     *
     * @param message message describing error
     * @param errnum system error number
     */
    serial_error(const char* message, int errnum = 0) noexcept;
    
    /**
     * Destroys this instance.
     */
    virtual ~serial_error() noexcept;
    
    /**
     * Get the message describing the error.
     *
     * @return message
     */
    virtual const char* what() const noexcept;
    
    /**
     * Get system error code.
     *
     * @return error code
     */
    long error_code();
    
private:
    std::string _message;
    long _code;
};
