/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB device configuration
 */

#ifndef USB_CONF_H
#define USB_CONF_H

#include <libopencm3/usb/usbd.h>

#define DATA_OUT_1 0x01
#define DATA_IN_1 0x81
#define COMM_IN_1 0x82

#define USB_SERIAL_NUM_LENGTH 24

usbd_device *usb_conf_init();
void usb_set_serial_number(const char *serial);

#endif
