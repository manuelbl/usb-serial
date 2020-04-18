/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB device implementation
 */

#ifndef USB_DEV_H
#define USB_DEV_H

#include <libopencm3/usb/usbd.h>

/**
 * Number of bytes received from host
 * @param addr endpoint address
 * @return number of bytes
 */
uint16_t usb_dev_get_rx_count(uint8_t addr);

/**
 * Copy received data to ring buffer.
 * 
 * The data is copied to the ring buffer starting at the head index.
 * If the data does not fit between the head and the end of buffer,
 * it will wrap around and continue at the start of the buffer.
 * The updated head index is returned.
 * 
 * @param addr endpoint addr
 * @param buf start of ring buffer
 * @param size size of ring buffer
 * @param head index to head of data
 * @return updated head index
 */
int usb_dev_copy_from_pm(uint8_t addr, uint8_t *buf, int size, int head);

#endif
