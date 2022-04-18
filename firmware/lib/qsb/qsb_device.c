//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// This file contains USB device code independent of the USB peripheral.
//

#include "qsb_private.h"
#include <string.h>

// Initializes USB devices
qsb_device* qsb_dev_init(qsb_port port, const qsb_device_desc* device_desc,
    const qsb_config_desc* config_descs, const char* const* strings, int num_strings, uint8_t* control_buffer,
    uint16_t control_buffer_size)
{
    qsb_device* device = port();

    device->desc = device_desc;
    device->config = config_descs;
    device->strings = strings;
    device->num_strings = num_strings;
    device->ctrl_buf = control_buffer;
    device->ctrl_buf_len = control_buffer_size;

    device->active_ep_callback = 0xff;

    device->ep_callbacks[0][QSB_TRANSACTION_SETUP] = qsb_internal_control_setup;
    device->ep_callbacks[0][QSB_TRANSACTION_OUT] = qsb_internal_control_out;
    device->ep_callbacks[0][QSB_TRANSACTION_IN] = qsb_internal_control_in;

    for (int i = 0; i < QSB_MAX_SET_CONFIG_CALLBACKS; i++)
        device->user_callback_set_config[i] = NULL;

    return device;
}

void qsb_dev_register_reset_callback(qsb_device* device, void (*callback)(void))
{
    device->user_callback_reset = callback;
}

void qsb_dev_register_suspend_callback(qsb_device* device, void (*callback)(void))
{
    device->user_callback_suspend = callback;
}

void qsb_dev_register_resume_callback(qsb_device* device, void (*callback)(void))
{
    device->user_callback_resume = callback;
}

void qsb_dev_register_sof_callback(qsb_device* device, void (*callback)(void))
{
    device->user_callback_sof = callback;
}

void qsb_internal_dev_reset(qsb_device* usbd_dev)
{
    usbd_dev->current_config = 0;
    qsb_dev_ep_setup(usbd_dev, 0, QSB_ENDPOINT_ATTR_CONTROL, usbd_dev->desc->bMaxPacketSize0, NULL);
    qsb_internal_dev_set_address(usbd_dev, 0);

    if (usbd_dev->user_callback_reset)
        usbd_dev->user_callback_reset();
}
