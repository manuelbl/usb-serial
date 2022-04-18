//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Implementation of basic USB full-speed device (excl. BTABLE access)
//

#include "qsb_config.h"

#if QSB_ARCH == QSB_ARCH_FSDEV && !defined(QSB_FSDEV_DBL_BUF)

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

    // Assign address and type
    qsb_ep_set_all_rw_bits(ep, typelookup[type] | ep);

    if (is_tx || ep == 0) {
        qsb_fsdev_setup_buf_tx(ep, qsb_offset_tx, buffer_size, &dev->pm_top);
        qsb_ep_dtog_tx_clear(ep);

        if (ep != 0)
            dev->ep_callbacks[ep][QSB_TRANSACTION_IN] = callback;

        qsb_ep_stat_tx_set(ep, USB_EP_STAT_TX_NAK);
    }

    if (!is_tx) {
        qsb_fsdev_setup_buf_rx(ep, qsb_offset_rx, buffer_size, &dev->pm_top);
        qsb_ep_dtog_rx_clear(ep);

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
    }
    dev->pm_top = PM_TOP_INIT + 2 * dev->desc->bMaxPacketSize0;
    dev->ep_state_rx[0] = 0;
}

void qsb_dev_ep_stall_set(__attribute__((unused)) qsb_device* dev, uint8_t addr, uint8_t stall)
{
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
    dev->ep_state_rx[ep] = 1;

    qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_NAK);
}

void qsb_dev_ep_unpause(qsb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (qsb_endpoint_is_tx(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    dev->ep_state_rx[ep] = 0;

    // if this method is called from a user callback, the endpoint status
    // will be set after the callback
    if (dev->active_ep_callback == addr)
        return;

    qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
}

uint16_t qsb_dev_ep_transmit_avail(__attribute__((unused)) qsb_device* dev, uint8_t addr)
{
    uint8_t ep = qsb_endpoint_num(addr);
    uint32_t ep_val = USB_EP(ep);
    if ((ep_val & USB_EP_CTR_TX) != 0)
        return 0; // pending callback
    return (ep_val & USB_EP_STAT_TX) == USB_EP_STAT_TX_VALID ? 0 : 64;
}

int qsb_dev_ep_transmit_packet(__attribute__((unused)) qsb_device* dev, uint8_t addr, const uint8_t* buf, int len)
{
    uint8_t ep = qsb_endpoint_num(addr);
    uint32_t ep_val = USB_EP(ep);
    if ((ep_val & USB_EP_CTR_TX) != 0)
        return -1; // pending callback
    if ((ep_val & USB_EP_STAT_TX) == USB_EP_STAT_TX_VALID)
        return -1; // endpoint is transmitting

    qsb_fsdev_copy_to_pma(ep, qsb_offset_tx, buf, len);
    qsb_ep_stat_tx_set(ep, USB_EP_STAT_TX_VALID);

    return len;
}

uint16_t qsb_dev_ep_read_packet(__attribute__((unused)) qsb_device* dev, uint8_t addr, uint8_t* buf, uint16_t len)
{
    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;

    if ((USB_EP(ep) & USB_EP_STAT_RX) == USB_EP_STAT_RX_VALID)
        return 0;

    return qsb_fsdev_copy_from_pma(buf, len, ep, qsb_offset_rx);
}

static inline void ep_callback(qsb_device* dev, uint8_t addr, uint8_t type, qsb_buf_desc_offset offset)
{
    uint8_t ep = qsb_endpoint_num(addr);
    if (dev->ep_callbacks[ep][type] == NULL)
        return;

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

        // correct RX transfer (SETUP or OUT)
        if ((ep_reg & USB_EP_CTR_RX) != 0) {
            qsb_ep_ctr_rx_clear(ep);
            int type = (ep_reg & USB_EP_SETUP) != 0 ? QSB_TRANSACTION_SETUP : QSB_TRANSACTION_OUT;
            ep_callback(dev, qsb_endpoint_addr_out(ep), type, qsb_offset_rx);
            if (!dev->ep_state_rx[ep])
                qsb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
        }

        // correct TX transfer (IN)
        if ((ep_reg & USB_EP_CTR_TX) != 0) {
            qsb_ep_ctr_tx_clear(ep);
            ep_callback(dev, qsb_endpoint_addr_in(ep), QSB_TRANSACTION_IN, qsb_offset_tx);
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
