/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * UART interface
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdlib.h>

#define UART_TX_BUF_LEN 1024
#define UART_RX_BUF_LEN 1024

enum class uart_state
{
    ready,
    transmitting,
    receiving
};


enum class uart_stopbits
{
    _1_0 = 0,
    _1_5 = 1,
    _2_0 = 2,
};


enum class uart_parity
{
    none = 0,
    odd = 1,
    even = 2,
};


/**
 * @brief UART implementation
 */
class uart_impl
{
public:
    /// Initialize UART
    void init();

    /**
     * @brief Submits the specified data for transmission.
     * 
     * The specified data is added to the transmission
     * buffer and transmitted asynchronously.
     * 
     * @param data pointer to byte array
     * @param len length of byte array
     */
    void transmit(const uint8_t *data, size_t len);

    /**
     * @brief Copies data from the receive buffer into the specified array.
     * 
     * The copied data is removed from the buffer.
     * 
     * @param data pointer to byte array
     * @param len length of the byte array
     * @return number of bytes copied to the byte array
     */
    size_t copy_rx_data(uint8_t *data, size_t len);

    /**
     * @brief Returns the length of received data in the receive buffer
     * 
     * @return length, in number of bytes
     */
    size_t rx_data_len();

    /**
     * @brief Returns the available space in the transmit buffer
     * 
     * @return space, in number of bytes
     */
    size_t tx_data_avail();

    /// Called when a chunk of data has been transmitted
    void on_tx_complete();

    /// Update (turn on/off) the RX/TX LEDs if needed
    void update_leds();

    /**
     * @brief Updates RTS (output signal)
     * 
     * The output signal is asserted if the receive buffer has room for more data
     * (is below the high-water mark) and `ready` is `true`.
     * 
     * @param ready `true` if USB connection is ready, `false` otherwise.
     */
    void update_rts(bool ready);

    /**
     * @brief Sets DTR (output signal)
     * 
     * @param asserted `true` if asserted, `false` if not asserted
     */
    void set_dtr(bool asserted);

    /**
     * @brief Returns if DSR (input signal) is asserted
     *
     * @return `true` if asserted, `false` otherwise
     */
    bool dsr();

    /**
     * @brief Returns if DCD (input signal) is asserted
     *
     * @return `true` if asserted, `false` otherwise
     */
    bool dcd();

    /**
     * @brief Sets the line coding.
     * 
     * @param baudrate baud rate, in bps
     * @param databits data bits per byte
     * @param stopbits length of stop period, in bits
     * @param parity type of party bit
     */ 
    void set_coding(int baudrate, int databits, uart_stopbits stopbits, uart_parity parity);

    /**
     * @brief Gets the baud rate.
     * 
     * @return baud rate, in bps
     */
    int baudrate() { return _baudrate; }

    /**
     * @brief Gets the data bits per byte.
     * 
     * @return number of bits
     */
    int databits() { return _databits; }

    /**
     * @brief Gets the length of the stop period
     * 
     * @return length, in bits
     */
    uart_stopbits stopbits() { return _stopbits; }

    /**
     * @brief Gets the type of parity bit
     * 
     * @return parity type
     */
    uart_parity parity() { return _parity; }

private:
    void start_transmit();

    static void clear_high_bit(uint8_t* buf, int buf_len);

    // Buffer for data to be transmitted via UART
    //  *  0 <= head < buf_len
    //  *  0 <= tail < buf_len
    //  *  head == tail => empty
    //  *  head + 1 == tail => full (modulo UART_TX_BUF_LEN)
    // `tx_buf_head` points to the positions where the next character
    // should be inserted. `tx_buf_tail` points to the character after
    // the last character that has been transmitted.
    uint8_t tx_buf[UART_TX_BUF_LEN];
    volatile int tx_buf_head;
    volatile int tx_buf_tail;

    // The number of bytes currently being transmitted
    volatile int tx_size;

    // Buffer of data received via UART
    //  *  0 <= head < buf_len
    //  *  0 <= tail < buf_len
    //  *  head == tail => empty
    //  *  head + 1 == tail => full (modulo UART_RX_BUF_LEN)
    // `rx_buf_head` points to the positions where the next character
    // should be inserted. `rx_buf_tail` points to the character after
    // the last character that has been transmitted.
    uint8_t rx_buf[UART_RX_BUF_LEN];
    // volatile int rx_buf_head: : managed by circular DMA controller
    volatile int rx_buf_tail;

    int _baudrate;
    int _databits;
    uart_stopbits _stopbits;
    uart_parity _parity;

    bool rx_led_timeout_active;
    bool tx_led_timeout_active;
    uint32_t rx_led_off_timeout;
    uint32_t tx_led_off_timeout;
    int rx_led_head;
    int rx_high_water_mark;

    volatile uart_state tx_state;
};

/// Global UART instance
extern uart_impl uart;

#endif
