/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB serial implementation
 */

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "usb_cdc.h"
#include <libopencm3/usb/cdc.h>

class usb_serial_impl
{
public:
    void config();
    void get_line_coding(struct usb_cdc_line_coding *line_coding);
    void set_line_coding(struct usb_cdc_line_coding *line_coding);
    void set_control_line_state(uint16_t state);
    uint8_t serial_state();
    void notify_serial_state();

    void out_cb(usbd_device *dev, uint8_t ep);
    void in_cb(usbd_device *dev, uint8_t ep);
    void poll();
    void update_nak();

private:
    void notify_serial_state(uint8_t state);

    bool is_usb_tx;
    uint8_t last_serial_state;
};

extern usb_serial_impl usb_serial;

#endif
