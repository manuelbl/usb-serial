//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Manipulation functions for USB endpoint registers.
//

#pragma once

#include "qusb_fsdev.h"

// The USB endpoint register is difficult to modify as it contains bits with four different behaviors:
//
// 1. R/W behavior: The bits of EP_TYPE, EP_KIND and EP_ADDR take the value written to them.
//    If other bits have to be modified, they have to be read and written without change.
// 2. Toggle behavior: The bits of DTOG_RX, STAT_RX, DTOG_TX and STAT_TX are toggled by writing 1.
//    If other bits have to be modified, they have to be written as 0.
// 3. Clear behavior: The bits of CTR_RX and CTR_TX can only be reset (by writing 0).
//    If other bits have to be modified, they have to be written as 1.
// 4. Read-only behavior: The EP_SETUP bit is read-only. Writes to it are ignored.
//
// But it gets worse. In double buffering mode, STAT_RX and STAT_TX will sometimes be masked as NAK,
// thus they read as NAK even though internally they are set to VALID. In order to change their value,
// the invisible internal value is relevant for determining the bits to toggle. In addition, the SW_BUF
// bit must also be toggled for the change to become effective.

/**
 * Set endpoint bits with toggle behavior to specified value.
 *
 * The bit mask specifies the bits to be affected. Other bits are not changed.
 *
 * @param ep endpoint
 * @param val new value
 * @param mask bit mask
 */
static inline void qusb_ep_set_toggle_bits(uint8_t ep, uint32_t val, uint32_t mask)
{
    // get bits with r/w behavior that need to be written without change plus affected bits
    uint32_t reg = USB_EP(ep) & (USB_EP_RW_BITS_MSK | mask);
    // set bits that need to be written 1 to not change
    reg |= USB_EP_W0_BITS_MSK;
    // set bits to be toggled
    reg ^= val;
    // write resulting value
    USB_EP(ep) = reg;
}

/**
 * Set endpoints bits with RW behavior.
 *
 * All bits with RW behavior are affected. The CTR bits are cleared.
 * All other bits are unaffected.
 *
 * @param ep endpoint
 * @param val new value
 */
static inline void qusb_ep_set_all_rw_bits(uint8_t ep, uint32_t val)
{
    // set bits that need to be written 1 to not change
    uint32_t reg = USB_EP_W0_BITS_MSK;
    // set bits to be updated
    reg |= val;
    // write resulting value
    USB_EP(ep) = reg;
}

/// Set endpoint RX status to specified value
static inline void qusb_ep_stat_rx_set(uint8_t ep, uint32_t stat)
{
    qusb_ep_set_toggle_bits(ep, stat, USB_EP_STAT_RX);
}

/// Set endpoint TX status to specified value
static inline void qusb_ep_stat_tx_set(uint8_t ep, uint32_t stat)
{
    qusb_ep_set_toggle_bits(ep, stat, USB_EP_STAT_TX);
}

/// Enable double-buffering for endpoint
static inline void qusb_ep_dbl_buf_set(uint8_t ep)
{
    qusb_ep_set_all_rw_bits(ep, USB_EP_KIND_DBL_BUF | USB_EP_TYPE_BULK | ep);
}

/// Clear the DTOG_RX bit
static inline void qusb_ep_dtog_rx_clear(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, 0, USB_EP_DTOG_RX);
}

/// Clear the DTOG_TX bit
static inline void qusb_ep_dtog_tx_clear(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, 0, USB_EP_DTOG_TX);
}

/// Clear the SW_BUF_RX bit
static inline void qusb_ep_sw_buf_rx_clear(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, 0, USB_EP_SW_BUF_RX);
}

/// Set the SW_BUF_RX bit
static inline void qusb_ep_sw_buf_rx_set(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, USB_EP_SW_BUF_RX, USB_EP_SW_BUF_RX);
}

/// Clear the SW_BUF_TX bit
static inline void qusb_ep_sw_buf_tx_clear(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, 0, USB_EP_SW_BUF_TX);
}

/// Set the SW_BUF_TX bit
static inline void qusb_ep_sw_buf_tx_set(uint8_t ep)
{
    qusb_ep_set_toggle_bits(ep, USB_EP_SW_BUF_TX, USB_EP_SW_BUF_TX);
}

/// Toggle the SW_BUF_RX bit
static inline void qusb_ep_sw_buf_rx_toggle(uint8_t ep)
{
    // This function is only used in double buffering mode so KIND_DBL_BUF and TYPE_BULK is assumed.
    USB_EP(ep) = USB_EP_KIND_DBL_BUF | USB_EP_TYPE_BULK | ep | USB_EP_W0_BITS_MSK | USB_EP_SW_BUF_RX;
}

/// Toggle the SW_BUF_TX bit
static inline void qusb_ep_sw_buf_tx_toggle(uint8_t ep)
{
    // This function is only used in double buffering mode so KIND_DBL_BUF and TYPE_BULK is assumed.
    USB_EP(ep) = USB_EP_KIND_DBL_BUF | USB_EP_TYPE_BULK | ep | USB_EP_W0_BITS_MSK | USB_EP_SW_BUF_TX;
}

/// Clear the CTR_RX bit
static inline void qusb_ep_ctr_rx_clear(uint8_t ep)
{
    USB_EP(ep) = (USB_EP(ep) & USB_EP_RW_BITS_MSK) | USB_EP_CTR_TX;
}

/// Clear the CTR_TX bit
static inline void qusb_ep_ctr_tx_clear(uint8_t ep)
{
    USB_EP(ep) = (USB_EP(ep) & USB_EP_RW_BITS_MSK) | USB_EP_CTR_RX;
}
