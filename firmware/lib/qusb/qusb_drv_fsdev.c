//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Implementation of basic USB full-speed device (excl. BTABLE access)
//

#include "qusb_config.h"

#if QUSB_ARCH == QUSB_ARCH_FSDEV && !defined(QUSB_FSDEV_DBL_BUF)

#include "qusb_fsdev.h"
#include "qusb_drv_fsdev_btable.h"
#include "qusb_fsdev_ep.h"
#include "qusb_private.h"
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/rcc.h>

#define USBD_PM_TOP 0x40

static qusb_device* create_port_fs(void);

qusb_port qusb_port_fs = create_port_fs;

// Allocate static space of USB device data structure
static qusb_device device_fsdev;

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

    // Assign address and type
    qusb_ep_set_all_rw_bits(ep, typelookup[type] | ep);

    if (is_tx || ep == 0) {
        qusb_fsdev_setup_buf_tx(ep, qusb_offset_tx, max_size, &dev->pm_top);
        qusb_ep_dtog_tx_clear(ep);

        if (ep != 0)
            dev->ep_callbacks[ep][QUSB_TRANSACTION_IN] = callback;

        qusb_ep_stat_tx_set(ep, USB_EP_STAT_TX_NAK);
    }

    if (!is_tx) {
        qusb_fsdev_setup_buf_rx(ep, qusb_offset_rx, max_size, &dev->pm_top);
        qusb_ep_dtog_rx_clear(ep);

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
    }
    dev->pm_top = USBD_PM_TOP + 2 * dev->desc->bMaxPacketSize0;
}

void qusb_dev_ep_stall_set(__attribute__((unused)) qusb_device* dev, uint8_t addr, uint8_t stall)
{
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
    dev->ep_state_rx[ep] = 1;

    // if this method is called from a user callback, the endpoint status
    // will be set after the callback
    if (dev->active_ep_callback == addr)
        return;

    qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_NAK);
}

void qusb_dev_ep_unpause(qusb_device* dev, uint8_t addr)
{
    // It does not make sense to force NAK on IN endpoints
    if (QUSB_ENDPOINT_IS_TX(addr))
        return;

    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;
    dev->ep_state_rx[ep] = 0;

    // if this method is called from a user callback, the endpoint status
    // will be set after the callback
    if (dev->active_ep_callback == addr)
        return;

    qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
}

uint16_t qusb_dev_ep_write_avail(__attribute__((unused)) qusb_device* dev, uint8_t addr)
{
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    uint32_t ep_val = USB_EP(ep);
    if ((ep_val & USB_EP_CTR_TX) != 0)
        return 0; // pending callback
    return (ep_val & USB_EP_STAT_TX) == USB_EP_STAT_TX_VALID ? 0 : 64;
}

uint16_t qusb_dev_ep_write_packet(__attribute__((unused)) qusb_device* dev, uint8_t addr, const uint8_t* buf, uint16_t len)
{
    uint8_t ep = QUSB_ENDPOINT_NUM(addr);
    uint32_t ep_val = USB_EP(ep);
    if ((ep_val & USB_EP_CTR_TX) != 0)
        return 0; // pending callback
    if ((ep_val & USB_EP_STAT_TX) == USB_EP_STAT_TX_VALID)
        return 0; // endpoint is transmitting

    qusb_fsdev_copy_to_pma(ep, qusb_offset_tx, buf, len);
    qusb_ep_stat_tx_set(ep, USB_EP_STAT_TX_VALID);

    return len;
}

uint16_t qusb_dev_ep_read_packet(__attribute__((unused)) qusb_device* dev, uint8_t addr, uint8_t* buf, uint16_t len)
{
    // Since IN bit is not set, ep equals addr
    uint8_t ep = addr;

    if ((USB_EP(ep) & USB_EP_STAT_RX) == USB_EP_STAT_RX_VALID)
        return 0;

    return qusb_fsdev_copy_from_pma(buf, len, ep, qusb_offset_rx);
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

    if (istr & USB_ISTR_CTR) {
        uint8_t ep = istr & USB_ISTR_EP_ID;
        uint8_t type;

        if ((istr & USB_ISTR_DIR) != 0) {
            // OUT or SETUP?
            type = (USB_EP(ep) & USB_EP_SETUP) != 0 ? QUSB_TRANSACTION_SETUP : QUSB_TRANSACTION_OUT;
            qusb_ep_ctr_rx_clear(ep);
        } else {
            type = QUSB_TRANSACTION_IN;
            qusb_ep_ctr_tx_clear(ep);
        }

        if (dev->ep_callbacks[ep][type]) {
            uint8_t offset = type == QUSB_TRANSACTION_IN ? qusb_offset_tx : qusb_offset_rx;
            dev->active_ep_callback = type == QUSB_TRANSACTION_IN ? QUSB_ENDPOINT_ADDR_IN(ep) : QUSB_ENDPOINT_ADDR_OUT(ep);
            dev->ep_callbacks[ep][type](dev, ep, qusb_fsdev_get_len(ep, offset));
            dev->active_ep_callback = 0xff;
            if ((istr & USB_ISTR_DIR) != 0 && !dev->ep_state_rx[ep])
                qusb_ep_stat_rx_set(ep, USB_EP_STAT_RX_VALID);
        }
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

    // enable SOF intrupt only if callback has been registered
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
