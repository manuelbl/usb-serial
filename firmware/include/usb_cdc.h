/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB CDC Implementation
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <libopencm3/usb/usbd.h>

#define CDCACM_PACKET_SIZE 64

extern usbd_device *usb_device;

void usb_cdc_init();
uint16_t usb_cdc_get_config();
int usb_cdc_get_dtr();

#endif
