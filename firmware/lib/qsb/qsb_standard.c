//
// QSB USB Device Library for libopencm3
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

#include "qsb_private.h"
#include "qsb_windows.h"
#include <string.h>

void qsb_dev_register_set_config_callback(qsb_device* dev, qsb_dev_set_config_callback_fn callback)
{
    for (int i = 0; i < QSB_MAX_SET_CONFIG_CALLBACKS; i++) {
        if (dev->user_callback_set_config[i] != NULL) {
            if (dev->user_callback_set_config[i] == callback)
                return;
            continue;
        }

        dev->user_callback_set_config[i] = callback;
        return;
    }
}

void qsb_dev_register_set_altsetting_callback(qsb_device* dev, qsb_dev_set_altsetting_callback_fn callback)
{
    dev->user_callback_set_altsetting = callback;
}

#define APPEND_TO_DESC(data_ptr, data_len)  \
    memcpy(buf_end, data_ptr, data_len);  \
    buf_end += data_len;

/**
 * Builds the config descriptor in the data structures expected by the host.
 *
 * Even if the host requests only part of the descriptor, this function will always
 * create the full descriptor. So the buffer must be sufficiently large.
 * It is the callers responsibility to reduce the size if needed.
 * 
 * @param dev USB device
 * @param index configuration index
 * @param buf buffer for result
 * @return length of descriptor 
 */
static uint16_t build_config_descriptor(qsb_device* dev, uint32_t index, uint8_t* buf)
{
    uint8_t* buf_end = buf;

    // append config descriptor
    const qsb_config_desc* cfg = &dev->config[index];
    APPEND_TO_DESC(cfg, cfg->bLength)

    // for each interface...
    for (int i = 0; i < cfg->bNumInterfaces; i++) {

        // append interface association descriptor, if any
        if (cfg->interface[i].iface_assoc) {
            const qsb_iface_assoc_desc* assoc = cfg->interface[i].iface_assoc;
            APPEND_TO_DESC(assoc, assoc->bLength)
        }

        // for each alternate setting...
        for (int j = 0; j < cfg->interface[i].num_altsetting; j++) {
            const qsb_interface_desc* iface = &cfg->interface[i].altsetting[j];

            // append interface descriptor
            APPEND_TO_DESC(iface, iface->bLength)

            // append extra bytes (function descriptors)
            if (iface->extra) {
                APPEND_TO_DESC(iface->extra, iface->extralen)
            }

            // for each endpoint...
            for (int k = 0; k < iface->bNumEndpoints; k++) {
                const qsb_endpoint_desc* ep = &iface->endpoint[k];
                APPEND_TO_DESC(ep, ep->bLength)

                // append extra bytes (class specific)
                if (ep->extra) {
                    APPEND_TO_DESC(ep->extra, ep->extralen)
                }
            }
        }
    }

    uint32_t length = buf_end - buf;

    // use memcpy() as buffer might not be word aligned
    memcpy(buf + 2, &length, sizeof(uint16_t));

    return length;
}

#undef APPEND_TO_DESC


#if QSB_STR_ENC == QSB_STR_ENC_UTF8

// fill string descriptor with given string (converting from UTF-8 to UTF-16)
static void fill_string_desc(const char* str, qsb_string_desc* desc, uint16_t* len)
{
    int max_len = *len - 1;
    uint16_t* target = desc->wData;
    int num_bytes = 2;
    
    while (true) {
        
        // decode UTF-8 codepoint
        uint32_t codepoint = (unsigned char)*str++;
        if (codepoint == 0)
            break;
        
        // number of leading 1s indicates number of bytes for codepoint
        // (acutally, number of bytes - 1 is computed; and -1 is returned for a single byte)
        int num_ext_bytes = __builtin_clz(~codepoint & 0xff) - 25;
        
        // clear upper bits (indicating length)
        codepoint &= ~(0x0f80 >> num_ext_bytes);
        
        // add extension bytes
        while (num_ext_bytes > 0) {
            uint32_t ext_byte = (unsigned char)*str++;
            codepoint = (codepoint << 6) | (ext_byte & 0x3f);
            num_ext_bytes--;
        }
        
        // encode UTF-16 code units
        if (codepoint < 0x010000) {
            if (num_bytes < max_len) *target++ = (uint16_t)codepoint;
            num_bytes += 2;
        } else {
            codepoint -= 0x10000;
            if (num_bytes < max_len) *target++ = (uint16_t)((codepoint >> 10) + 0xd800);
            num_bytes += 2;
            if (num_bytes < max_len) *target++ = (uint16_t)((codepoint & 0x03ff) + 0xdc00);
            num_bytes += 2;
        }
    }

    // length of data in this response
    *len = imin(num_bytes, *len);
    // length of full descriptor
    desc->bLength = num_bytes;
}

#else

// fill string descriptor with given string (converting from Latin-1 to UTF-16)
static void fill_string_desc(const char* str, qsb_string_desc* desc, uint16_t* len)
{
    // UTF-16 encoding: 2 bytes per character plus length and type
    int size = strlen(str) * 2 + 2;
    desc->bLength = size;

    if (size > *len) {
        size = *len;
    } else {
        *len = size;
    }

    // copy and convert from Latin-1 to UTF-16
    size = size / 2 - 1;
    for (int i = 0; i < size; i++)
        desc->wData[i] = str[i];
}

#endif

// get string descriptor
static qsb_request_return_code get_string_descriptor(
    qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len, int descr_idx)
{
    qsb_string_desc* desc = (qsb_string_desc*)*buf;

    if (descr_idx == 0) {
        // language ID descriptor
        desc->wData[0] = QSB_LANGID_ENGLISH_US;
        desc->bLength = sizeof(desc->wData[0]) + sizeof(desc->bLength) + sizeof(desc->bDescriptorType);
        *len = imin(*len, desc->bLength);

#if QSB_WIN_WCID == 1

    } else if (descr_idx == QSB_WIN_MSFT_WCID_STR_IDX) {

        return qsb_internal_win_get_msft_string_desc(buf, len);
#endif

    } else {
        int array_idx = descr_idx - 1;

        if (!dev->strings)
            return QSB_REQ_NOTSUPP; // device doesn't support strings

        if (array_idx >= dev->num_strings)
            return QSB_REQ_NOTSUPP; // string index is out of range

        if (req->wIndex != QSB_LANGID_ENGLISH_US)
            return QSB_REQ_NOTSUPP; // request for language ID other than English-US

        fill_string_desc(dev->strings[array_idx], desc, len);
    }

    desc->bDescriptorType = QSB_DT_STRING;

    return QSB_REQ_HANDLED;
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
static qsb_request_return_code get_descriptor(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    int descr_idx = descriptor_index(req->wValue);

    switch (descriptor_type(req->wValue)) {
    case QSB_DT_DEVICE:
        *buf = (uint8_t*)dev->desc;
        *len = imin(*len, dev->desc->bLength);
        return QSB_REQ_HANDLED;

    case QSB_DT_CONFIGURATION:
        *len = imin(*len, build_config_descriptor(dev, descr_idx, *buf));
        return QSB_REQ_HANDLED;

    case QSB_DT_STRING:
        return get_string_descriptor(dev, req, buf, len, descr_idx);

#if QSB_BOS == 1
    case QSB_DT_BOS:
        if (descr_idx == 0)
            return qsb_internal_bos_request_get_desc(dev, req, buf, len);
        return QSB_REQ_NOTSUPP;
#endif

    default:
        return QSB_REQ_NOTSUPP;
    }
}

// set device address
static qsb_request_return_code set_address(
    __attribute__((unused)) qsb_device* dev, __attribute__((unused)) qsb_setup_data* req,
    __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    // The actual address is only latched at the STATUS IN stage.
    if (req->bmRequestType != 0 || req->wValue >= 128)
        return QSB_REQ_NOTSUPP;

#if QSB_ARCH == QSB_ARCH_DWC
    // Special workaround for STM32F10[57] that require the address
    // to be set here. This is undocumented!
    qsb_internal_dev_set_address(dev, req->wValue);
#endif

    return QSB_REQ_HANDLED;
}

// set the device configuration
static qsb_request_return_code set_configuration(qsb_device* dev, __attribute__((unused)) qsb_setup_data* req,
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
            return QSB_REQ_NOTSUPP;
    }

    dev->current_config = found_index + 1;

    if (dev->current_config > 0) {
        const qsb_config_desc* cfg = &dev->config[dev->current_config - 1];

        // reset all alternate settings configuration
        for (int i = 0; i < cfg->bNumInterfaces; i++) {
            if (cfg->interface[i].cur_altsetting)
                *cfg->interface[i].cur_altsetting = 0;
        }
    }

    // Reset all endpoints
    qsb_internal_ep_reset(dev);

    if (dev->user_callback_set_config[0]) {
        // Reset (flush) control callbacks. These will be reregistered by the user handler
        for (int i = 0; i < QSB_MAX_CONTROL_CALLBACKS; i++)
            dev->user_control_callback[i].cb = NULL;

        for (int i = 0; i < QSB_MAX_SET_CONFIG_CALLBACKS; i++) {
            if (dev->user_callback_set_config[i] != NULL)
                dev->user_callback_set_config[i](dev, req->wValue);
        }
    }

    return QSB_REQ_HANDLED;
}

// get the device configuration
static qsb_request_return_code get_configuration(
    qsb_device* dev, __attribute__((unused)) qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 1)
        *len = 1;

    if (dev->current_config > 0) {
        const qsb_config_desc* cfg = &dev->config[dev->current_config - 1];
        (*buf)[0] = cfg->bConfigurationValue;
    } else {
        (*buf)[0] = 0;
    }

    return QSB_REQ_HANDLED;
}

// get the current alternative interface setting
static qsb_request_return_code set_interface(
    qsb_device* dev, qsb_setup_data* req, __attribute__((unused)) uint8_t** buf, uint16_t* len)
{
    const qsb_config_desc* cfx = &dev->config[dev->current_config - 1];

    if (req->wIndex >= cfx->bNumInterfaces)
        return QSB_REQ_NOTSUPP;

    const qsb_interface* iface = &cfx->interface[req->wIndex];

    if (req->wValue >= iface->num_altsetting)
        return QSB_REQ_NOTSUPP;

    if (iface->cur_altsetting) {
        *iface->cur_altsetting = req->wValue;
    } else if (req->wValue > 0) {
        return QSB_REQ_NOTSUPP;
    }

    if (dev->user_callback_set_altsetting)
        dev->user_callback_set_altsetting(dev, req->wIndex, req->wValue);

    *len = 0;

    return QSB_REQ_HANDLED;
}

// get the current alternative interface setting
static qsb_request_return_code get_interface(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    const qsb_config_desc* cfx = &dev->config[dev->current_config - 1];

    if (req->wIndex >= cfx->bNumInterfaces)
        return QSB_REQ_NOTSUPP;

    uint8_t* cur_altsetting = cfx->interface[req->wIndex].cur_altsetting;
    *len = 1;
    (*buf)[0] = cur_altsetting != NULL ? *cur_altsetting : 0;

    return QSB_REQ_HANDLED;
}

// get device status (always returns 0)
static qsb_request_return_code device_get_status(
    __attribute__((unused)) qsb_device* dev, __attribute__((unused)) qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    // bit 0: self powered
    // bit 1: remote wakeup
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return QSB_REQ_HANDLED;
}

// get interface status (always returns 0)
static qsb_request_return_code interface_get_status(
    __attribute__((unused)) qsb_device* dev, __attribute__((unused)) qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 2)
        *len = 2;
    (*buf)[0] = 0;
    (*buf)[1] = 0;

    return QSB_REQ_HANDLED;
}

// get endpoint status (halted/stalled)
static qsb_request_return_code endpoint_get_status(
    qsb_device* dev, __attribute__((unused)) qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (*len > 2)
        *len = 2;
    (*buf)[0] = qsb_dev_ep_stall_get(dev, req->wIndex) ? 1 : 0;
    (*buf)[1] = 0;

    return QSB_REQ_HANDLED;
}

// stall endpoint
static qsb_request_return_code endpoint_stall(
    qsb_device* dev, qsb_setup_data* req, __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    qsb_dev_ep_stall_set(dev, req->wIndex, 1);

    return QSB_REQ_HANDLED;
}

// unstall endpoint
static qsb_request_return_code endpoint_unstall(
    qsb_device* dev, qsb_setup_data* req, __attribute__((unused)) uint8_t** buf, __attribute__((unused)) uint16_t* len)
{
    qsb_dev_ep_stall_set(dev, req->wIndex, 0);
    return QSB_REQ_HANDLED;
}

// Handle standard control requests for devices
qsb_request_return_code qsb_internal_standard_request_device(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qsb_request_return_code (*command)(qsb_device * dev, qsb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QSB_REQ_CLEAR_FEATURE:
    case QSB_REQ_SET_FEATURE:
        // if (req->wValue == QSB_FEAT_DEVICE_REMOTE_WAKEUP) {
        // 	// TODO: device wakeup
        // }

        // if (req->wValue == QSB_FEAT_TEST_MODE) {
        // 	// TODO: test mode
        // }
        break;

    case QSB_REQ_SET_ADDRESS:
        command = set_address;
        break;

    case QSB_REQ_SET_CONFIGURATION:
        command = set_configuration;
        break;

    case QSB_REQ_GET_CONFIGURATION:
        command = get_configuration;
        break;

    case QSB_REQ_GET_DESCRIPTOR:
        command = get_descriptor;
        break;

    case QSB_REQ_GET_STATUS:
        command = device_get_status;
        break;

    case QSB_REQ_SET_DESCRIPTOR:
        // SET_DESCRIPTOR is optional and not implemented
        break;
    }

    if (!command)
        return QSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests for interfaces
qsb_request_return_code qsb_internal_standard_request_interface(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qsb_request_return_code (*command)(qsb_device * dev, qsb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QSB_REQ_CLEAR_FEATURE:
    case QSB_REQ_SET_FEATURE:
        // interface features can be implemented using user callbacks
        break;

    case QSB_REQ_GET_INTERFACE:
        command = get_interface;
        break;

    case QSB_REQ_SET_INTERFACE:
        command = set_interface;
        break;

    case QSB_REQ_GET_STATUS:
        command = interface_get_status;
        break;
    }

    if (!command)
        return QSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests for endpoints
qsb_request_return_code qsb_internal_standard_request_endpoint(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    qsb_request_return_code (*command)(qsb_device * dev, qsb_setup_data * req, uint8_t * *buf, uint16_t * len) = NULL;

    switch (req->bRequest) {
    case QSB_REQ_CLEAR_FEATURE:
        if (req->wValue == QSB_FEAT_ENDPOINT_HALT)
            command = endpoint_unstall;
        break;

    case QSB_REQ_SET_FEATURE:
        if (req->wValue == QSB_FEAT_ENDPOINT_HALT)
            command = endpoint_stall;
        break;

    case QSB_REQ_GET_STATUS:
        command = endpoint_get_status;
        break;

    case QSB_REQ_SET_SYNCH_FRAME:
        // FIXME: SYNCH_FRAME is not implemented.
        //
        // SYNCH_FRAME is used for synchronization of isochronous
        // endpoints which are not yet implemented.
        break;
    }

    if (!command)
        return QSB_REQ_NOTSUPP;

    return command(dev, req, buf, len);
}

// Handle standard control requests
qsb_request_return_code qsb_internal_standard_request(qsb_device* dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    // class and vendor types have to be handled by user callbacks
    if ((req->bmRequestType & QSB_REQ_TYPE_TYPE_MASK) != QSB_REQ_TYPE_STANDARD)
        return QSB_REQ_NOTSUPP;

    // dispatch by type
    switch (req->bmRequestType & QSB_REQ_TYPE_RECIPIENT_MASK) {
    case QSB_REQ_TYPE_DEVICE:
        return qsb_internal_standard_request_device(dev, req, buf, len);

    case QSB_REQ_TYPE_INTERFACE:
        return qsb_internal_standard_request_interface(dev, req, buf, len);

    case QSB_REQ_TYPE_ENDPOINT:
        return qsb_internal_standard_request_endpoint(dev, req, buf, len);

    default:
        return QSB_REQ_NOTSUPP;
    }
}
