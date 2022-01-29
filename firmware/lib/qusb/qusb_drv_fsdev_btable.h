//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Functions for accessing the buffer descriptor table (BTABLE) and packet memory (PMA).
//

#pragma once

#include "qusb_private.h"

/**
 * Setup buffer in packet memory for RX.
 *
 * The allocated buffer will start at the packet memory address specified
 * by `pm_top`. The address will then be updated to point to the new top.
 *
 * @param ep Endpoint address without direction bit
 * @param offset Offset within buffer descriptor table (0 or 1)
 * @param size buffer size (in bytes)
 * @param pm_top packet memory top address (relative to base address)
 */
void qusb_fsdev_setup_buf_rx(uint8_t ep, qusb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top);

/**
 * Setup buffer in packet memory for TX.
 *
 * The allocated buffer will start at the packet memory address specified
 * by `pm_top`. The address will then be updated to point to the new top.
 *
 * @param ep Endpoint address without direction bit
 * @param offset Offset within buffer descriptor table (0 or 1)
 * @param size buffer size (in bytes)
 * @param pm_top packet memory top address (relative to base address)
 */
void qusb_fsdev_setup_buf_tx(uint8_t ep, qusb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top);

/**
 * Get the length of the transmitted or received data.
 *
 * @param ep Endpoint address without direction bit
 * @param offset Offset within buffer descriptor table (0 or 1)
 * @return length (in bytes)
 */
uint32_t qusb_fsdev_get_len(uint8_t ep, qusb_buf_desc_offset offset);

/**
 * Copy a data buffer to USB packet memory.
 *
 * @param ep Endpoint address without direction bit (target)
 * @param offset Offset within buffer descriptor table (0 or 1)
 * @param buf pointer to data buffer (source)
 * @param len length of data
 */
void qusb_fsdev_copy_to_pma(uint8_t ep, qusb_buf_desc_offset offset, const uint8_t* buf, uint32_t len);

/**
 * Copy USB packet memory into a data buffer.
 *
 * @param buf pointer to data buffer (target)
 * @param len length of data buffer
 * @param ep Endpoint address without direction bit (source)
 * @param offset Offset within buffer descriptor table (0 or 1)
 * @return number of bytes copied
 */

uint32_t qusb_fsdev_copy_from_pma(uint8_t* buf, uint32_t len, uint8_t ep, qusb_buf_desc_offset offset);
