//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// This file contains USB device code independent of the USB peripheral.
//

#include "qusb_private.h"
#include <string.h>

// Initializes USB devices
qusb_device* qusb_dev_init(qusb_port port, const qusb_device_desc* device_desc,
    const qusb_config_desc* config_descs, const char* const* strings, int num_strings, uint8_t* control_buffer,
    uint16_t control_buffer_size)
{
    qusb_device* device = port();

    device->desc = device_desc;
    device->config = config_descs;
    device->strings = strings;
    device->num_strings = num_strings;
    device->extra_string_idx = 0;
    device->extra_string = NULL;
    device->ctrl_buf = control_buffer;
    device->ctrl_buf_len = control_buffer_size;

    device->active_ep_callback = 0xff;

    device->ep_callbacks[0][QUSB_TRANSACTION_SETUP] = _qusb_control_setup;
    device->ep_callbacks[0][QUSB_TRANSACTION_OUT] = _qusb_control_out;
    device->ep_callbacks[0][QUSB_TRANSACTION_IN] = _qusb_control_in;

    for (int i = 0; i < MAX_USER_SET_CONFIG_CALLBACK; i++)
        device->user_callback_set_config[i] = NULL;

    return device;
}

void qusb_dev_register_reset_callback(qusb_device* device, void (*callback)(void))
{
    device->user_callback_reset = callback;
}

void qusb_dev_register_suspend_callback(qusb_device* device, void (*callback)(void))
{
    device->user_callback_suspend = callback;
}

void qusb_dev_register_resume_callback(qusb_device* device, void (*callback)(void))
{
    device->user_callback_resume = callback;
}

void qusb_dev_register_sof_callback(qusb_device* device, void (*callback)(void))
{
    device->user_callback_sof = callback;
}

void qusb_dev_register_extra_string(qusb_device* device, int index, const char* string)
{
    device->extra_string = string;
    device->extra_string_idx = string != NULL ? index : 0;
}

void _qusb_dev_reset(qusb_device* usbd_dev)
{
    usbd_dev->current_config = 0;
    qusb_dev_ep_setup(usbd_dev, 0, QUSB_ENDPOINT_ATTR_CONTROL, usbd_dev->desc->bMaxPacketSize0, NULL);
    _qusb_dev_set_address(usbd_dev, 0);

    if (usbd_dev->user_callback_reset)
        usbd_dev->user_callback_reset();
}
