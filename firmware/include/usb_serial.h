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

    void out_cb(usbd_device *dev);
    void in_cb();
    void poll();
    void update_nak();

    uint32_t tick;

private:
    void notify_serial_state(uint8_t state);

    bool is_usb_tx;
    bool is_tx_high_water;
    uint8_t last_serial_state;
    uint32_t rx_data_tick;
};

extern usb_serial_impl usb_serial;

#endif
