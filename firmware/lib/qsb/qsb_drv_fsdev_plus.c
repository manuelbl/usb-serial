//
// QSB USB Device Library for libopencm3
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

#include "qsb_config.h"

#if QSB_ARCH == QSB_ARCH_FSDEV && defined(QSB_FSDEV_DBL_BUF)

#include "qsb_fsdev.h"
#include "qsb_drv_fsdev_btable.h"
#include "qsb_fsdev_ep.h"
#include "qsb_private.h"
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/rcc.h>
#include <stdlib.h>

// Initial program memory top making space for the buffer descriptors (BTABLE). 
static const uint32_t PM_TOP_INIT = QSB_NUM_ENDPOINTS * 8;

static qsb_device* create_port_fs(void);

qsb_port qsb_port_fs = create_port_fs;


// Allocate static space of USB device data structure
static qsb_device device_fsdev;

/// TX endpoint state
typedef enum ep_state_tx_e {
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
typedef enum ep_state_rx_e {
    /// Single buffering, ready state
    sgl_buf_ready,
    /// Single buffering, paused state (NAK)
    sgl_buf_paused,
    /// Double buffering, ready state, next packet at offset 0
    dbl_buf_ready_0,
    /// Double buffering, ready state, next packet at offset 1
    dbl_buf_ready_1,
    /// Double buffering, paused state, next packet at offset 0
    dbl_buf_paused_0,
    /// Double buffering, paused state, next packet at offset 1
    dbl_buf_paused_1
} ep_state_rx_e;

// Initialize the USB device controller hardware of the STM32
qsb_device* create_port_fs(void)
{
    rcc_periph_clock_enable(RCC_USB);
    USB_CNTR = 0;
    USB_BTABLE = 0;
    USB_ISTR = 0;

    // Enable RESET, SUSPEND, RESUME and CTR interrupts.
    USB_CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
#if QSB_FSDEV_SUBTYPE >= 3
    USB_BCDR = USB_BCDR_DPPU;
#endif
    return &device_fsdev;
}

void qsb_internal_dev_set_address(__attribute__((unused)) qsb_device* dev, uint8_t addr)
{
    USB_DADDR = (addr & USB_DADDR_ADDR) | USB_DADDR_EF;
}

void qsb_dev_ep_setup(qsb_device* dev, uint8_t addr, uint32_t type, int buffer_size, qsb_dev_ep_callback_fn callback)
{
    // Translate USB standard type codes to STM32
    const uint16_t typelookup[] = {
        [QSB_ENDPOINT_ATTR_CONTROL] = USB_EP_TYPE_CONTROL,
        [QSB_ENDPOINT_ATTR_ISOCHRONOUS] = USB_EP_TYPE_ISO,
        [QSB_ENDPOINT_ATTR_BULK] = USB_EP_TYPE_BULK,
        [QSB_ENDPOINT_ATTR_INTERRUPT] = USB_EP_TYPE_INTERRUPT,
    };
    bool is_tx = qsb_endpoint_is_tx(addr);
    uint8_t ep = qsb_endpoint_num(addr);
    bool is_dbl_buf = false;
    if (type == QSB_ENDPOINT_ATTR_BULK && buffer_size > 64) {
        is_dbl_buf = true;
        buffer_size = 64;
    }

    // Assign address and type
    qsb_ep_set_all_rw_bits(ep, ep | typelookup[type] | (is_dbl_buf ? USB_EP_KIND_DBL_BUF : 0));

    if (is_tx || ep == 0) {
        dev->ep_state_tx[ep] = is_dbl_buf ? dbl_buf_en_0_pkts : sgl_buf_0_pkts;
        qsb_fsdev_setup_buf_tx(ep, qsb_offset_tx, buffer_size, &dev->pm_top);
        qsb_ep_dtog_tx_clear(ep);

        if (is_dbl_buf) {
            qsb_ep_sw_buf_tx_clear(ep);
            qsb_fsdev_setup_buf_tx(ep, qsb_offset_db1, buffer_size, &dev->pm_top);
        }

        if (ep != 0)
            dev->ep_callbacks[ep][QSB_TRANSACTION_IN] = callback;

        qsb_ep_stat_tx_set(ep, is_dbl_buf ? USB_EP_STAT_TX_VALID : USB_EP_STAT_TX_NAK);
    }

    if (!is_tx) {
        dev->ep_state_rx[ep] = is_dbl_buf ? dbl_buf_ready_0 : sgl_buf_ready;
        qsb_fsdev_setup_buf_rx(ep, qsb_offset_rx, buffer_size, &dev->pm_top);
        qsb_ep_dtog_rx_clear(ep);

        if (is_dbl_buf) {
            qsb_fsdev_setup_buf_rx(ep, qsb_offset_db0, buffer_size, &dev->pm_top);
            qsb_ep_sw_buf_rx_set(ep);
        }

        if (ep != 0)
            dev->ep_callbacks[ep][QSB_TRANSACTION_OUT] = callback;

        qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
    }
}

void qsb_internal_ep_reset(qsb_device* dev)
{
    // Reset all endpoints
    for (int i = 1; i < 8; i++) {
        qsb_ep_stat_tx_set(i, USB_EP_STAT_TX_DISABLED);
        qsb_ep_stat_rx_set(i, USB_EP_STAT_RX_DISABLED);
        dev->ep_state_rx[i] = 0;
        dev->ep_state_tx[i] = 0;
        dev->ep_outstanig_rx_acks[i] = 0;
    }
    dev->pm_top = PM_TOP_INIT + 2 * dev->desc->bMaxPacketSize0;
}

void qsb_dev_ep_stall_set(__attribute__((unused)) qsb_device* dev, uint8_t addr, uint8_t stall)
{
    // TODO: support for double buffering
    if (addr == 0)
        qsb_ep_stat_tx_set(0, stall ? USB_EP_STAT_TX_STALL : USB_EP_STAT_TX_NAK);

    bool is_tx = qsb_endpoint_is_tx(addr);
    uint8_t ep = qsb_endpoint_num(addr);
    if (is_tx) {
        qsb_ep_stat_tx_set(ep, stall ? USB_EP_STAT_TX_STALL : USB_EP_STAT_TX_NAK);

        // Reset to DATA0 if clearing stall condition
        if (!stall)
            qsb_ep_dtog_tx_clear(ep);

    } else {
        // Reset to DATA0 if clearing stall condition
        if (!stall)
            qsb_ep_dtog_rx_clear(ep);

        qsb_ep_stat_rx_set(ep, stall ? USB_EP_STAT_RX_STALL : USB_EP_STAT_RX_VALID);
    }
}

uint8_t qsb_dev_ep_stall_get(__attribute__((unused)) qsb_device* dev, uint8_t addr)
{
    bool is_tx = qsb_endpoint_is_tx(addr);
    uint8_t ep = qsb_endpoint_num(addr);
    if (is_tx) {
        if ((USB_EP(ep) & USB_EP_STAT_TX) == USB_EP_STAT_TX_STALL)
            return 1;

    } else {
        if ((USB_EP(ep) & USB_EP_STAT_RX) == USB_EP_STAT_RX_STALL)
            return 1;
    }
    return 0;
}

void qsb_dev_ep_pause(qsb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (qsb_endpoint_is_tx(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    switch (dev->ep_state_rx[ep]) {
    case sgl_buf_ready:
        dev->ep_state_rx[ep] = sgl_buf_paused;
        qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_NAK);
        break;

    case dbl_buf_ready_0:
    case dbl_buf_ready_1:
        dev->ep_state_rx[ep] += 2;
        //uint32_t reg = ep | USB_EP_KIND_DBL_BUF | USB_EP_TYPE_BULK | USB_EP_W0_BITS_MSK | (USB_EP_STAT_RX_NAK ^ USB_EP_STAT_RX_VALID);
        // USB_EP_SW_BUF_RX needs to be toggled to make the change stick. It's toggled here, if this function is called outside
        // an endpoint callback. Otherwise, it will be toggled by qsb_dev_poll() after the callback returns.
        // if (dev->active_ep_callback != addr)
        //     reg |= USB_EP_SW_BUF_RX;
        //USB_EP(ep) = reg;
        break;
    }
}

void qsb_dev_ep_unpause(qsb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (qsb_endpoint_is_tx(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    switch (dev->ep_state_rx[ep]) {
    case sgl_buf_paused:
        dev->ep_state_rx[ep] = sgl_buf_ready;
        if (dev->active_ep_callback != addr)
            qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
        break;

    case dbl_buf_paused_0:
    case dbl_buf_paused_1:
        dev->ep_state_rx[ep] -= 2;
        while (dev->ep_outstanig_rx_acks[ep] > 0) {
            qsb_ep_sw_buf_rx_toggle(ep);
            dev->ep_outstanig_rx_acks[ep] -= 1;
        }
        break;
    }
}

uint16_t qsb_dev_ep_transmit_avail(qsb_device* dev, uint8_t addr)
{
    uint8_t ep = qsb_endpoint_num(addr);
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

int qsb_dev_ep_transmit_packet(qsb_device* dev, uint8_t addr, const uint8_t* buf, int len)
{
    uint8_t ep = qsb_endpoint_num(addr);
    ep_state_tx_e state = dev->ep_state_tx[ep];

    if (state == sgl_buf_0_pkts) {
        // submit a single packet in single buffering mode
        len = imin(len, 64);
        qsb_fsdev_copy_to_pma(ep, qsb_offset_tx, buf, len);
        dev->ep_state_tx[ep] = sgl_buf_1_pkt;
        qsb_ep_stat_tx_set(ep, USB_EP_STAT_TX_VALID);

    } else if (state == dbl_buf_en_0_pkts || state == dbl_buf_en_1_pkt) {
        // submit one or two packets in double buffering mode
        uint8_t offset = (USB_EP(ep) & USB_EP_SW_BUF_TX) == 0 ? qsb_offset_db0 : qsb_offset_db1;
        len = imin(len, 64);
        qsb_fsdev_copy_to_pma(ep, offset, buf, len);
        dev->ep_state_tx[ep] = state + 1;
        qsb_ep_sw_buf_tx_toggle(ep);

    } else {
        // busy with a single packet in single buffering
        // or two packets in double buffering mode
        return -1;
    }

    return len;
}

uint16_t qsb_dev_ep_read_packet(qsb_device* dev, uint8_t addr, uint8_t* buf, uint16_t len)
{
    if (dev->active_ep_callback != addr)
        return 0; // call is only valid from within user callback of this endpoint

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    uint32_t ep_reg = USB_EP(ep);
    uint8_t offset = qsb_offset_rx;
    if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0)
            offset = dev->ep_state_rx[ep] & 1;

    return qsb_fsdev_copy_from_pma(buf, len, ep, offset);
}

static inline void ep_callback(qsb_device* dev, uint8_t ep, uint8_t type, uint8_t offset)
{
    if (dev->ep_callbacks[ep][type] == NULL)
        return;

    uint8_t addr = type == QSB_TRANSACTION_IN ? qsb_endpoint_addr_in(ep) : qsb_endpoint_addr_out(ep);
    dev->active_ep_callback = addr;
    dev->ep_callbacks[ep][type](dev, addr, qsb_fsdev_get_len(ep, offset));
    dev->active_ep_callback = 0xff;
}

void qsb_dev_poll(qsb_device* dev)
{
    uint32_t istr = USB_ISTR;

    if (istr & USB_ISTR_RESET) {
        USB_ISTR = ~USB_ISTR_RESET;
        dev->pm_top = PM_TOP_INIT;
        qsb_internal_dev_reset(dev);
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
                qsb_ep_ctr_rx_clear(0);
                ep_callback(dev, 0, QSB_TRANSACTION_SETUP, qsb_offset_rx);
                qsb_ep_stat_rx_set(0, USB_EP_STAT_RX_VALID);
            
            // Regular OUT transfer
            } else {
                qsb_ep_ctr_rx_clear(ep);
                uint8_t offset = qsb_offset_rx;
                if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0)
                    offset = dev->ep_state_rx[ep] & 1;

                ep_callback(dev, ep, QSB_TRANSACTION_OUT, offset);

                ep_state_rx_e rx_state = dev->ep_state_rx[ep];
                if (rx_state == sgl_buf_ready) {
                    qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
                } else if (rx_state >= dbl_buf_ready_0 && rx_state <= dbl_buf_paused_1) {
                    if (rx_state <= dbl_buf_ready_1)
                        qsb_ep_sw_buf_rx_toggle(ep);
                    else
                        dev->ep_outstanig_rx_acks[ep] += 1;
                    dev->ep_state_rx[ep] ^= 1;
                }
            }
        }        

        // correct TX transfer
        if ((ep_reg & USB_EP_CTR_TX) != 0) {
            qsb_ep_ctr_tx_clear(ep);

            if (dev->ep_state_tx[ep] != sgl_buf_0_pkts && dev->ep_state_tx[ep] != dbl_buf_en_0_pkts)
                dev->ep_state_tx[ep]--;

            uint8_t offset = qsb_offset_tx;
            if ((ep_reg & USB_EP_KIND_DBL_BUF) != 0 && (ep_reg & USB_EP_SW_BUF_TX) == 0)
                offset = qsb_offset_db1;
            ep_callback(dev, ep, QSB_TRANSACTION_IN, offset);
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

#if QSB_FSDEV_SUBTYPE >= 3
void qsb_dev_disconnect(__attribute__((unused)) qsb_device* dev, bool disconnected)
{
    if (disconnected) {
        USB_BCDR |= USB_BCDR_DPPU;
    } else {
        USB_BCDR &= ~USB_BCDR_DPPU;
    }
}
#endif

#endif
