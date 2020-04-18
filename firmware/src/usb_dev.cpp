/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB device implementation
 */

#include "usb_dev.h"
#include <libopencm3/stm32/st_usbfs.h>

uint16_t usb_dev_get_rx_count(uint8_t addr)
{
    if ((*USB_EP_REG(addr) & USB_EP_RX_STAT) == USB_EP_RX_STAT_VALID)
        return 0;

    return USB_GET_EP_RX_COUNT(addr) & 0x3ff;
}

int usb_dev_copy_from_pm(uint8_t addr, uint8_t *buf, int size, int head)
{
    const volatile uint16_t *pm = (uint16_t *)USB_GET_EP_RX_BUFF(addr);
    uint16_t len = USB_GET_EP_RX_COUNT(addr) & 0x3ff;

    bool odd = len & 1;
    len >>= 1;

    for (; len; len--)
    {
        uint16_t value = *pm;

        buf[head] = value;
        head++;
        if (head >= size)
            head = 0;

        buf[head] = value >> 8;
        head++;
        if (head >= size)
            head = 0;

#if defined(STM32F0)
        pm++;
#elif defined(STM32F1)
        pm += 2;
#endif
    }

    if (odd)
    {
        buf[head] = *(uint8_t *)pm;
        head++;
        if (head >= size)
            head = 0;
    }

    return head;
}
