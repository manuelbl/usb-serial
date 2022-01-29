//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Implementation of enhanced USB full-speed device with support for double buffering
// (excl. BTABLE access)
//

#include "qusb_config.h"

#if QUSB_ARCH == QUSB_ARCH_FSDEV && defined(QUSB_FSDEV_DBL_BUF)

#include "qusb_fsdev.h"
#include "qusb_drv_fsdev_btable.h"
#include "qusb_fsdev_ep.h"
#include "qusb_private.h"
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/rcc.h>
#include <stdlib.h>

#define USBD_PM_TOP 0x40

static qusb_device* create_port_fs(void);

qusb_port qusb_port_fs = create_port_fs;


// Allocate static space of USB device data structure
static qusb_device device_fsdev;

/// TX endpoint state
typedef enum {
    /// Single buffering, no packets submitted
    sgl_buf_0_pkts = 0,
    /// Single buffering, 1 packet submitted
    sgl_buf_1_pkt = 1,
    /// Double buffering enabled, no packets submitted
    dbl_buf_en_0_pkts = 2,
    /// Double buffering enabled, 1 packet submitted
    dbl_buf_en_1_pkt = 3,
    /// Double buffering enabled, 2 packets submitted
    dbl_buf_en_2_pkts = 4
} ep_state_tx_e;

/// RX endpoint state
typedef enum {
    /// Single buffering, ready state
    sgl_buf_ready,
    /// Single buffering, paused state (NAK)
    sgl_buf_paused,
    /// Double buffering, ready state, next packet in buffer 0
    dbl_buf_ready_0,
    /// Double buffering, ready state, next packet in buffer 1
    dbl_buf_ready_1,
    /// Double buffering, paused state, next packet in buffer 0 (NAK)
    dbl_buf_paused_0,
    /// Double buffering, paused state, next packet in buffer 1 (NAK)
    dbl_buf_paused_1
} ep_state_rx_e;

// Initialize the USB device controller hardware of the STM32
qusb_device* create_port_fs(void)
{
    rcc_periph_clock_enable(RCC_USB);
    USB_CNTR = 0;
    USB_BTABLE = 0;
    USB_ISTR = 0;

    // Enable RESET, SUSPEND, RESUME and CTR interrupts.
    USB_CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
#if QUSB_FSDEV_SUBTYPE >= 3
    USB_BCDR = USB_BCDR_DPPU;
#endif
    return &device_fsdev;
}

void _qusb_dev_set_address(__attribute__((unused)) qusb_device* dev, uint8_t addr)
{
    USB_DADDR = (addr & USB_DADDR_ADDR) | USB_DADDR_EF;
}

void qusb_dev_ep_setup(qusb_device* dev, uint8_t addr, uint8_t type, uint16_t max_size, qusb_dev_ep_callback_fn callback)
{
    // Translate USB standard type codes to STM32
    const uint16_t typelookup[] = {
        [QUSB_ENDPOINT_ATTR_CONTROL] = USB_EP_TYPE_CONTROL,
        [QUSB_ENDPOINT_ATTR_ISOCHRONOUS] = USB_EP_TYPE_ISO,
        [QUSB_ENDPOINT_ATTR_BULK] = USB_EP_TYPE_BULK,
        [QUSB_ENDPOINT_ATTR_INTERRUPT] = USB_EP_TYPE_INTERRUPT,
    };
    bool is_tx = QUSB_ENDPOINT_IS_TX(addr);
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    bool is_dbl_buf = false;
    if (type == QUSB_ENDPOINT_ATTR_BULK && max_size > 64) {
        is_dbl_buf = true;
        max_size = 64;
    }

    // Assign address and type
    qusb_ep_set_all_rw_bits(ep, ep | typelookup[type] | (is_dbl_buf ? USB_EP_KIND_DBL_BUF : 0));

    if (is_tx || ep == 0) {
        dev->ep_state_tx[ep] = is_dbl_buf ? dbl_buf_en_0_pkts : sgl_buf_0_pkts;
        qusb_fsdev_setup_buf_tx(ep, qusb_offset_tx, max_size, &dev->pm_top);
        qusb_ep_dtog_tx_clear(ep);

        if (is_dbl_buf) {
            qusb_ep_sw_buf_tx_clear(ep);
            qusb_fsdev_setup_buf_tx(ep, qusb_offset_db1, max_size, &dev->pm_top);
        }

        if (ep != 0)
            dev->ep_callbacks[ep][QUSB_TRANSACTION_IN] = callback;

        qusb_ep_stat_tx_set(ep, is_dbl_buf ? USB_EP_STAT_TX_VALID : USB_EP_STAT_TX_NAK);
    }

    if (!is_tx) {
        dev->ep_state_rx[ep] = is_dbl_buf ? dbl_buf_ready_0 : sgl_buf_ready;
        qusb_fsdev_setup_buf_rx(ep, qusb_offset_rx, max_size, &dev->pm_top);
        qusb_ep_dtog_rx_clear(ep);

        if (is_dbl_buf) {
            qusb_fsdev_setup_buf_rx(ep, qusb_offset_db0, max_size, &dev->pm_top);
            qusb_ep_sw_buf_rx_set(ep);
        }

        if (ep != 0)
            dev->ep_callbacks[ep][QUSB_TRANSACTION_OUT] = callback;

        qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
    }
}

void _qusb_ep_reset(qusb_device* dev)
{
    // Reset all endpoints
    for (int i = 1; i < 8; i++) {
        qusb_ep_stat_tx_set(i, USB_EP_STAT_TX_DISABLED);
        qusb_ep_stat_rx_set(i, USB_EP_STAT_RX_DISABLED);
        dev->ep_state_rx[i] = 0;
        dev->ep_state_tx[i] = 0;
    }
    dev->pm_top = USBD_PM_TOP + 2 * dev->desc->bMaxPacketSize0;
}

void qusb_dev_ep_stall_set(__attribute__((unused)) qusb_device* dev, uint8_t addr, uint8_t stall)
{
    // TODO: support for double buffering
    if (addr == 0)
        qusb_ep_stat_tx_set(0, stall ? USB_EP_STAT_TX_STALL : USB_EP_STAT_TX_NAK);

    bool is_tx = QUSB_ENDPOINT_IS_TX(addr);
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    if (is_tx) {
        qusb_ep_stat_tx_set(ep, stall ? USB_EP_STAT_TX_STALL : USB_EP_STAT_TX_NAK);

        // Reset to DATA0 if clearing stall condition
        if (!stall)
            qusb_ep_dtog_tx_clear(ep);

    } else {
        // Reset to DATA0 if clearing stall condition
        if (!stall)
            qusb_ep_dtog_rx_clear(ep);

        qusb_ep_stat_rx_set(ep, stall ? USB_EP_STAT_RX_STALL : USB_EP_STAT_RX_VALID);
    }
}

uint8_t qusb_dev_ep_stall_get(__attribute__((unused)) qusb_device* dev, uint8_t addr)
{
    bool is_tx = QUSB_ENDPOINT_IS_TX(addr);
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    if (is_tx) {
        if ((USB_EP(ep) & USB_EP_STAT_TX) == USB_EP_STAT_TX_STALL)
            return 1;

    } else {
        if ((USB_EP(ep) & USB_EP_STAT_RX) == USB_EP_STAT_RX_STALL)
            return 1;
    }
    return 0;
}

void qusb_dev_ep_pause(qusb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (QUSB_ENDPOINT_IS_TX(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    switch (dev->ep_state_rx[ep]) {
    case sgl_buf_ready:
        dev->ep_state_rx[ep] = sgl_buf_paused;
        qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_NAK);
        break;

    case dbl_buf_ready_0:
    case dbl_buf_ready_1:
        dev->ep_state_rx[ep] += 2;
        uint32_t reg = ep | USB_EP_KIND_DBL_BUF | USB_EP_TYPE_BULK | USB_EP_W0_BITS_MSK | (USB_EP_STAT_RX_NAK ^ USB_EP_STAT_RX_VALID);
        // USB_EP_SW_BUF_RX needs to be toggled to make the change stick. It's toggled here, if this function is called outside
        // an endpoint callback. Otherwise, it will be toggled by qusb_dev_poll() after the callback returns.
        if (dev->active_ep_callback != addr)
            reg |= USB_EP_SW_BUF_RX;
        USB_EP(ep) = reg;
        break;
    }
}

void qusb_dev_ep_unpause(qusb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (QUSB_ENDPOINT_IS_TX(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    switch (dev->ep_state_rx[ep]) {
    case sgl_buf_paused:
        dev->ep_state_rx[ep] = sgl_buf_ready;
        if (dev->active_ep_callback != addr)
            qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
        break;

    case dbl_buf_paused_0:
    case dbl_buf_paused_1:
        dev->ep_state_rx[ep] -= 2;
        uint32_t ep_val = USB_EP(ep);
        if ((ep_val & USB_EP_DTOG_RX) != 0) {
            qusb_ep_sw_buf_rx_set(ep);
        } else {
            qusb_ep_sw_buf_rx_clear(ep);
        }
        qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
        break;
    }
}

uint16_t qusb_dev_ep_write_avail(qusb_device* dev, uint8_t addr)
{
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    ep_state_tx_e dbl_buf_state = dev->ep_state_tx[ep];

    switch (dbl_buf_state) {
    case dbl_buf_en_0_pkts:
    case sgl_buf_0_pkts:
    case dbl_buf_en_1_pkt:
        return 64;
    default:
        return 0;
    }
}

uint16_t qusb_dev_ep_write_packet(qusb_device* dev, uint8_t addr, const uint8_t* buf, uint16_t len)
{
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    ep_state_tx_e state = dev->ep_state_tx[ep];

    if (state == sgl_buf_0_pkts) {
        // submit a single packet in single buffering mode
        len = min_u16(len, 64);
        qusb_fsdev_copy_to_pma(ep, qusb_offset_tx, buf, len);
        dev->ep_state_tx[ep] = sgl_buf_1_pkt;
        qusb_ep_stat_tx_set(ep, USB_EP_STAT_TX_VALID);

    } else if (state == dbl_buf_en_0_pkts || state == dbl_buf_en_1_pkt) {
        // submit one or two packets in double buffering mode
        uint8_t offset = (USB_EP(ep) & USB_EP_SW_BUF_TX) == 0 ? qusb_offset_db0 : qusb_offset_db1;
        len = min_u16(len, 64);
        qusb_fsdev_copy_to_pma(ep, offset, buf, len);
        dev->ep_state_tx[ep] = state + 1;
        qusb_ep_sw_buf_tx_toggle(ep);

    } else {
        // busy with a single packet in single buffering
        // or two packets in double buffering mode
        return 0x7fff;
    }

    return len;
}

uint16_t qusb_dev_ep_read_packet(qusb_device* dev, uint8_t addr, uint8_t* buf, uint16_t len)
{
    if (dev->active_ep_callback != addr)
        return 0; // call is only valid from within user callback of this endpoint

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    uint32_t ep_reg = USB_EP(ep);
    uint8_t offset = qusb_offset_rx;
    if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0)
            offset = dev->ep_state_rx[ep] & 1;

    return qusb_fsdev_copy_from_pma(buf, len, ep, offset);
}

static inline void ep_callback(qusb_device* dev, uint8_t ep, uint8_t type, uint8_t offset) {
    if (dev->ep_callbacks[ep][type] == NULL)
        return;

    uint8_t addr = type == QUSB_TRANSACTION_IN ? QUSB_ENDPOINT_ADDR_IN(ep) : QUSB_ENDPOINT_ADDR_OUT(ep);
    dev->active_ep_callback = addr;
    dev->ep_callbacks[ep][type](dev, addr, qusb_fsdev_get_len(ep, offset));
    dev->active_ep_callback = 0xff;
}

void qusb_dev_poll(qusb_device* dev)
{
    uint32_t istr = USB_ISTR;

    if (istr & USB_ISTR_RESET) {
        USB_ISTR = ~USB_ISTR_RESET;
        dev->pm_top = USBD_PM_TOP;
        _qusb_dev_reset(dev);
        return;
    }

    // correct transfer
    while ((istr & USB_ISTR_CTR) != 0) {
        uint8_t ep = istr & USB_ISTR_EP_ID;
        uint32_t ep_reg = USB_EP(ep);

        // correct RX transfer
        if ((ep_reg & USB_EP_CTR_RX) != 0) {
            
            // SETUP transfer
            if ((ep_reg & USB_EP_SETUP) != 0) {
                qusb_ep_ctr_rx_clear(0);
                ep_callback(dev, 0, QUSB_TRANSACTION_SETUP, qusb_offset_rx);
                qusb_ep_stat_rx_set(0, USB_EP_STAT_RX_VALID);
            
            // Regular OUT transfer
            } else {
                qusb_ep_ctr_rx_clear(ep);
                uint8_t offset = qusb_offset_rx;
                if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0)
                    offset = dev->ep_state_rx[ep] & 1;

                ep_callback(dev, ep, QUSB_TRANSACTION_OUT, offset);

                ep_state_rx_e rx_state = dev->ep_state_rx[ep];
                if (rx_state == sgl_buf_ready) {
                    qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
                } else if (rx_state >= dbl_buf_ready_0 && rx_state <= dbl_buf_paused_1) {
                    qusb_ep_sw_buf_rx_toggle(ep);
                    dev->ep_state_rx[ep] ^= 1;
                }
            }
        }        

        // correct TX transfer
        if ((ep_reg & USB_EP_CTR_TX) != 0) {
            qusb_ep_ctr_tx_clear(ep);

            if (dev->ep_state_tx[ep] != sgl_buf_0_pkts && dev->ep_state_tx[ep] != dbl_buf_en_0_pkts)
                dev->ep_state_tx[ep]--;

            uint8_t offset = qusb_offset_tx;
            if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0 && (ep_reg & USB_EP_SW_BUF_TX) == 0)
                offset = qusb_offset_db1;
            ep_callback(dev, ep, QUSB_TRANSACTION_IN, offset);
        }

        istr = USB_ISTR;
    }

    if (istr & USB_ISTR_SUSP) {
        USB_ISTR = ~USB_ISTR_SUSP;
        if (dev->user_callback_suspend)
            dev->user_callback_suspend();
    }

    if (istr & USB_ISTR_WKUP) {
        USB_ISTR = ~USB_ISTR_WKUP;
        if (dev->user_callback_resume)
            dev->user_callback_resume();
    }

    if (istr & USB_ISTR_SOF) {
        USB_ISTR = ~USB_ISTR_SOF;
        if (dev->user_callback_sof)
            dev->user_callback_sof();
    }

    // enable SOF interupt only if callback has been registered
    if (dev->user_callback_sof) {
        USB_CNTR |= USB_CNTR_SOFM;
    } else {
        USB_CNTR &= ~USB_CNTR_SOFM;
    }
}

#if QUSB_FSDEV_SUBTYPE >= 3
void qusb_dev_disconnect(__attribute__((unused)) qusb_device* dev, bool disconnected)
{
    if (disconnected) {
        USB_BCDR |= USB_BCDR_DPPU;
    } else {
        USB_BCDR &= ~USB_BCDR_DPPU;
    }
}
#endif

#endif
