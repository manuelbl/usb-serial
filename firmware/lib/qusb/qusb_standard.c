//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Implementation of the standard control requests.
//
// Very notably, this includes the code for building the device descriptor.
//

#include "qusb_private.h"
#include <string.h>

int qusb_dev_register_set_config_callback(qusb_device* dev, qusb_dev_set_config_callback_fn callback)
{
    for (int i = 0; i < MAX_USER_SET_CONFIG_CALLBACK; i++) {
        if (dev->user_callback_set_config[i]) {
            if (dev->user_callback_set_config[i] == callback)
                return 0;
            continue;
        }

        dev->user_callback_set_config[i] = callback;
        return 0;
    }

    return -1;
}

void qusb_dev_register_set_altsetting_callback(qusb_device* dev, qusb_dev_set_altsetting_callback_fn callback)
{
    dev->user_callback_set_altsetting = callback;
}

#define APPEND_TO_DESC(data_ptr, data_len)                                                                             \
    count = min_u32(buf_len, data_len);                                                                                \
    memcpy(buf_end, data_ptr, count);                                                                                  \
    buf_end += count;                                                                                                  \
    buf_len -= count;                                                                                                  \
    length += count;                                                                                                   \
    total_length += data_len;

// builds the config descriptor in the data structures expected by the host
static uint16_t build_config_descriptor(qusb_device* dev, uint32_t index, uint8_t* buf, uint32_t buf_len)
{
    uint8_t* buf_end = buf;
    uint32_t count;
    uint32_t length = 0;
    uint32_t total_length = 0;

    // append config descriptor
    const qusb_config_desc* cfg = &dev->config[index];
    APPEND_TO_DESC(cfg, cfg->bLength)

    // for each interface...
    for (int i = 0; i < cfg->bNumInterfaces; i++) {

        // append interface association descriptor, if any
        if (cfg->interface[i].iface_assoc) {
            const qusb_iface_assoc_desc* assoc = cfg->interface[i].iface_assoc;
            APPEND_TO_DESC(assoc, assoc->bLength)
        }

        // for each alternate setting...
        for (int j = 0; j < cfg->interface[i].num_altsetting; j++) {
            const qusb_interface_desc* iface = &cfg->interface[i].altsetting[j];

            // append interface descriptor
            APPEND_TO_DESC(iface, iface->bLength)

            // append extra bytes (function descriptors)
            if (iface->extra) {
                APPEND_TO_DESC(iface->extra, iface->extralen)
            }

            // for each endpoint...
            for (int k = 0; k < iface->bNumEndpoints; k++) {
                const qusb_endpoint_desc* ep = &iface->endpoint[k];
                APPEND_TO_DESC(ep, ep->bLength)

                // append extra bytes (class specific)
                if (ep->extra) {
                    APPEND_TO_DESC(ep->extra, ep->extralen)
                }
            }
        }
    }

    // use memcpy() as buffer might not be word aligned
    memcpy(buf + 2, &total_length, sizeof(uint16_t));

    return length;
}

#undef APPEND_TO_DESC

// fill string descriptor with given string
static inline void fill_string_desc(const char* str, qusb_string_desc* desc, uint16_t* len)
{
    // UTF-16 encoding: 2 bytes per character plus length and type
    int size = strlen(str) * 2 + 2;
    desc->bLength = size;

    if (size > *len) {
        size = *len;
    } else {
        *len = size;
    }

    size = size / 2 - 1;
    for (int i = 0; i < size; i++)
        desc->wData[i] = str[i];
}

// get string descriptor
static qusb_request_return_code get_string_descriptor(
    qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len, int descr_idx)
{
    qusb_string_desc* desc = (qusb_string_desc*)dev->ctrl_buf;

    if (descr_idx == 0) {
        // language ID descriptor
        desc->wData[0] = QUSB_LANGID_ENGLISH_US;
        desc->bLength = sizeof(desc->wData[0]) + sizeof(desc->bLength) + sizeof(desc->bDescriptorType);
        *len = min_u16(*len, desc->bLength);

    } else if (descr_idx == dev->extra_string_idx) {
        fill_string_desc(dev->extra_string, desc, len);

    } else {
        int array_idx = descr_idx - 1;

        if (!dev->strings)
            return QUSB_REQ_NOTSUPP; // device doesn't support strings

        if (array_idx >= dev->num_strings)
            return QUSB_REQ_NOTSUPP; // string index is out of range

        if (req->wIndex != QUSB_LANGID_ENGLISH_US)
            return QUSB_REQ_NOTSUPP; // request for language ID other than English-US

        fill_string_desc(dev->strings[array_idx], desc, len);
    }

    desc->bDescriptorType = QUSB_DT_STRING;
    *buf = (uint8_t*)desc;

    return QUSB_REQ_HANDLED;
}

static inline int descriptor_type(uint16_t wValue)
{
    return wValue >> 8;
}

static inline int descriptor_index(uint16_t wValue)
{
    return wValue & 0xFF;
}

// get device, configuration and string descriptor
static qusb_request_return_code get_descriptor(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    int descr_idx = descriptor_index(req->wValue);

    switch (descriptor_type(req->wValue)) {
    case QUSB_DT_DEVICE:
        *buf = (uint8_t*)dev->desc;
        *len = min_u16(*len, dev->desc->bLength);
        return QUSB_REQ_HANDLED;

    case QUSB_DT_CONFIGURATION:
        *buf = dev->ctrl_buf;
        *len = build_config_descriptor(dev, descr_idx, *buf, *len);
        return QUSB_REQ_HANDLED;

    case QUSB_DT_STRING:
        return get_string_descriptor(dev, req, buf, len, descr_idx);

    default:
        return QUSB_REQ_NOTSUPP;
    }
}

// set device address
static qusb_request_return_code set_address(
    __attribute__((unused)) qusb_device* dev, __attribute__((unused)) qusb_setup_data* req,
    __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    // The actual address is only latched at the STATUS IN stage.
    if (req->bmRequestType != 0 || req->wValue >= 128)
        return QUSB_REQ_NOTSUPP;

#if QUSB_ARCH == QUSB_ARCH_DWC
    // Special workaround for STM32F10[57] that require the address
    // to be set here. This is undocumented!
    _qusb_dev_set_address(dev, req->wValue);
#endif

    return QUSB_REQ_HANDLED;
}

// set the device configuration
static qusb_request_return_code set_configuration(qusb_device* dev, __attribute__((unused)) qusb_setup_data* req,
    __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    int found_index = -1;
    if (req->wValue > 0) {
        for (int i = 0; i < dev->desc->bNumConfigurations; i++) {
            if (req->wValue == dev->config[i].bConfigurationValue) {
                found_index = i;
                break;
            }
        }
        if (found_index < 0)
            return QUSB_REQ_NOTSUPP;
    }

    dev->current_config = found_index + 1;

    if (dev->current_config > 0) {
        const qusb_config_desc* cfg = &dev->config[dev->current_config - 1];

        // reset all alternate settings configuration
        for (int i = 0; i < cfg->bNumInterfaces; i++) {
            if (cfg->interface[i].cur_altsetting)
                *cfg->interface[i].cur_altsetting = 0;
        }
    }

    // Reset all endpoints
    _qusb_ep_reset(dev);

    if (dev->user_callback_set_config[0]) {
        // Reset (flush) control callbacks. These will be reregistered by the user handler
        for (int i = 0; i < MAX_USER_CONTROL_CALLBACK; i++)
            dev->user_control_callback[i].cb = NULL;

        for (int i = 0; i < MAX_USER_SET_CONFIG_CALLBACK; i++) {
            if (dev->user_callback_set_config[i])
                dev->user_callback_set_config[i](dev, req->wValue);
        }
    }

    return QUSB_REQ_HANDLED;
}

// get the device configuration
static qusb_request_return_code get_configuration(
    qusb_device* dev, __attribute__((unused)) qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 1)
        *len = 1;

    if (dev->current_config > 0) {
        const qusb_config_desc* cfg = &dev->config[dev->current_config - 1];
        (*buf)[0] = cfg->bConfigurationValue;
    } else {
        (*buf)[0] = 0;
    }

    return QUSB_REQ_HANDLED;
}

// get the current alternative interface setting
static qusb_request_return_code set_interface(
    qusb_device* dev, qusb_setup_data* req, __attribute__((unused)) uint8_t** buf, uint16_t* len)
{
    const qusb_config_desc* cfx = &dev->config[dev->current_config - 1];

    if (req->wIndex >= cfx->bNumInterfaces)
        return QUSB_REQ_NOTSUPP;

    const qusb_interface* iface = &cfx->interface[req->wIndex];

    if (req->wValue >= iface->num_altsetting)
        return QUSB_REQ_NOTSUPP;

    if (iface->cur_altsetting) {
        *iface->cur_altsetting = req->wValue;
    } else if (req->wValue > 0) {
        return QUSB_REQ_NOTSUPP;
    }

    if (dev->user_callback_set_altsetting)
        dev->user_callback_set_altsetting(dev, req->wIndex, req->wValue);

    *len = 0;

    return QUSB_REQ_HANDLED;
}

// get the current alternative interface setting
static qusb_request_return_code get_interface(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    const qusb_config_desc* cfx = &dev->config[dev->current_config - 1];

    if (req->wIndex >= cfx->bNumInterfaces)
        return QUSB_REQ_NOTSUPP;

    uint8_t* cur_altsetting = cfx->interface[req->wIndex].cur_altsetting;
    *len = 1;
    (*buf)[0] = cur_altsetting != NULL ? *cur_altsetting : 0;

    return QUSB_REQ_HANDLED;
}

// get device status (always returns 0)
static qusb_request_return_code device_get_status(
    __attribute__((unused)) qusb_device* dev, __attribute__((unused)) qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    // bit 0: self powered
    // bit 1: remote wakeup
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return QUSB_REQ_HANDLED;
}

// get interface status (always returns 0)
static qusb_request_return_code interface_get_status(
    __attribute__((unused)) qusb_device* dev, __attribute__((unused)) qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return QUSB_REQ_HANDLED;
}

// get endpoint status (halted/stalled)
static qusb_request_return_code endpoint_get_status(
    qusb_device* dev, __attribute__((unused)) qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 2)
        *len = 2;
    (*buf)[0] = qusb_dev_ep_stall_get(dev, req->wIndex) ? 1 : 0;
    (*buf)[1] = 0;

    return QUSB_REQ_HANDLED;
}

// stall endpoint
static qusb_request_return_code endpoint_stall(
    qusb_device* dev, qusb_setup_data* req, __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    qusb_dev_ep_stall_set(dev, req->wIndex, 1);

    return QUSB_REQ_HANDLED;
}

// unstall endpoint
static qusb_request_return_code endpoint_unstall(
    qusb_device* dev, qusb_setup_data* req, __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    qusb_dev_ep_stall_set(dev, req->wIndex, 0);
    return QUSB_REQ_HANDLED;
}

// Handle standard control requests for devices
qusb_request_return_code _qusb_standard_request_device(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qusb_request_return_code (*command)(qusb_device * dev, qusb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QUSB_REQ_CLEAR_FEATURE:
    case QUSB_REQ_SET_FEATURE:
        // if (req->wValue == QUSB_FEAT_DEVICE_REMOTE_WAKEUP) {
        // 	// TODO: device wakeup
        // }

        // if (req->wValue == QUSB_FEAT_TEST_MODE) {
        // 	// TODO: test mode
        // }
        break;

    case QUSB_REQ_SET_ADDRESS:
        command = set_address;
        break;

    case QUSB_REQ_SET_CONFIGURATION:
        command = set_configuration;
        break;

    case QUSB_REQ_GET_CONFIGURATION:
        command = get_configuration;
        break;

    case QUSB_REQ_GET_DESCRIPTOR:
        command = get_descriptor;
        break;

    case QUSB_REQ_GET_STATUS:
        command = device_get_status;
        break;

    case QUSB_REQ_SET_DESCRIPTOR:
        // SET_DESCRIPTOR is optional and not implemented
        break;
    }

    if (!command)
        return QUSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests for interfaces
qusb_request_return_code _qusb_standard_request_interface(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qusb_request_return_code (*command)(qusb_device * dev, qusb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QUSB_REQ_CLEAR_FEATURE:
    case QUSB_REQ_SET_FEATURE:
        // interface features can be implemented using user callbacks
        break;

    case QUSB_REQ_GET_INTERFACE:
        command = get_interface;
        break;

    case QUSB_REQ_SET_INTERFACE:
        command = set_interface;
        break;

    case QUSB_REQ_GET_STATUS:
        command = interface_get_status;
        break;
    }

    if (!command)
        return QUSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests for endpoints
qusb_request_return_code _qusb_standard_request_endpoint(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qusb_request_return_code (*command)(qusb_device * dev, qusb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QUSB_REQ_CLEAR_FEATURE:
        if (req->wValue == QUSB_FEAT_ENDPOINT_HALT)
            command = endpoint_unstall;
        break;

    case QUSB_REQ_SET_FEATURE:
        if (req->wValue == QUSB_FEAT_ENDPOINT_HALT)
            command = endpoint_stall;
        break;

    case QUSB_REQ_GET_STATUS:
        command = endpoint_get_status;
        break;

    case QUSB_REQ_SET_SYNCH_FRAME:
        // FIXME: SYNCH_FRAME is not implemented.
        //
        // SYNCH_FRAME is used for synchronization of isochronous
        // endpoints which are not yet implemented.
        break;
    }

    if (!command)
        return QUSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests
qusb_request_return_code _qusb_standard_request(qusb_device* dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    // class and vendor types have to be handled by user callbacks
    if ((req->bmRequestType & QUSB_REQ_TYPE_TYPE_MASK) != QUSB_REQ_TYPE_STANDARD)
        return QUSB_REQ_NOTSUPP;

    // dispatch by type
    switch (req->bmRequestType & QUSB_REQ_TYPE_RECIPIENT_MASK) {
    case QUSB_REQ_TYPE_DEVICE:
        return _qusb_standard_request_device(dev, req, buf, len);

    case QUSB_REQ_TYPE_INTERFACE:
        return _qusb_standard_request_interface(dev, req, buf, len);

    case QUSB_REQ_TYPE_ENDPOINT:
        return _qusb_standard_request_endpoint(dev, req, buf, len);

    default:
        return QUSB_REQ_NOTSUPP;
    }
}
