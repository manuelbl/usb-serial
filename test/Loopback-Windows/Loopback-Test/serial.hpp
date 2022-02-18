//
//  USB Serial
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Serial port class (for Windows).
//

#pragma once

#include <exception>
#include <string>


class serial_port {
public:
    /**
     * Create a new serial port instance.
     *
     * The serial port is in closed state.
     */
    serial_port();

    ~serial_port();
    
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
    void* _hComPort;
    void* _hEvent;
};


class serial_error : public std::exception {
public:
    serial_error(const char* message, unsigned long errnum = 0) noexcept;
    virtual ~serial_error() noexcept;
    virtual const char* what() const noexcept;
    long error_code();
    
private:
    std::string _message;
    long _code;
};

