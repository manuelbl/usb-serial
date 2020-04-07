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
    bits_1_0 = 0,
    bits_1_5 = 1,
    bits_2_0 = 2,
};


enum class uart_parity
{
    no_parity = 0,
    odd_parity = 1,
    even_parity = 2,
};


class uart_impl
{
public:
    /// Initialize UART
    void init();

    /**
     * Submits the specified data for transmission.
     * 
     * The specified data is added to the transmission
     * buffer and transmitted asynchronously.
     * 
     * @param data pointer to byte array
     * @param len length of byte array
     */
    void transmit(const uint8_t *data, size_t len);

    /**
     * Copy data from the receive buffer in the specified array.
     * 
     * The copied data is removed from the buffer.
     * 
     * @param data pointer to byte array
     * @param len length of the byte array
     * @return number of bytes copied to the byte array
     */
    size_t copy_rx_data(uint8_t *data, size_t len);

    /// Returns the number of bytes in the receive buffer
    size_t rx_data_len();

    /// Returns the number of bytes left in the transmission buffer
    size_t tx_data_avail();

    /// Called when a chunk of data has been transmitted
    void on_tx_complete();

    /// Update (turn on/off) the RX/TX LEDs if needed
    void update_leds();

    /// Update the state of the RTS signal
    void update_rts(bool ready);

    void set_dtr(bool asserted);
    bool dsr();
    bool dcd();

    void set_coding(int baudrate, int databits, uart_stopbits stopbits, uart_parity parity);
    int baudrate() { return _baudrate; }
    int databits() { return _databits; }
    uart_stopbits stopbits() { return _stopbits; }
    uart_parity parity() { return _parity; }

private:
    void start_transmit();

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

extern uart_impl uart;

#endif
