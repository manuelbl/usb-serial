//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Main header file to be included when using the Qusb library
//

#pragma once

#include "qusb_config.h"
#include "qusb_std_data.h"

#ifdef __cplusplus
extern "C" {
#endif 

/// Return codes for functions handling requests
typedef enum qusb_request_return_code {
    /// Request not supported
    QUSB_REQ_NOTSUPP = 0,
    /// Request handled
    QUSB_REQ_HANDLED = 1,
    /// Request not handled, pass on the next handler
    QUSB_REQ_NEXT_HANDLER = 2,
} qusb_request_return_code;

/// USB device (opaque data structure)
typedef struct _qusb_device qusb_device;

/// USB port (creator function)
typedef qusb_device* (*qusb_port)(void);


/// USB full-speed port (PA12 and PA11)
extern qusb_port qusb_port_fs;

#if QUSB_ARCH == QUSB_ARCH_DWC
/// USB high-speed port (TODO)
extern qusb_port qusb_port_hs;
#endif

/**
 * Initialize USB device.
 *
 * This function assumes that the USB clock has already been configured
 * for USB operation (usually for 48MHz).
 *
 * To place `strings` entirely into Flash/read-only memory, use
 * `static const * const strings[] = { ... };`
 * (note the double `const`).  The first `const` refers to the strings
 * while the second `const` refers to the array.
 *
 * @param port USB port
 * @param device_desc USB device descriptor. This must not be changed while the device is in use.
 * @param config_descs Array of USB configuration descriptors. These must not be changed while the device is in use. The
 * length of this array is determined by the `bNumConfigurations` field in the device descriptor.
 * @param strings Array of strings for USB string descriptors. The device, configuration and interface descriptors refer
 * to these string, e.g. in `iManufacturer`. Strings indexes are offset by 1: index 1 refers to strings[0].
 * @param num_strings Number of items in `strings` array.
 * @param control_buffer Array that will hold the data received during control requests with DATA stage
 * @param control_buffer_size Size of `control_buffer`
 * @return the USB device initialized for use
 */
qusb_device* qusb_dev_init(qusb_port port, const qusb_device_desc* device_desc,
    const qusb_config_desc* config_descs, const char* const* strings, int num_strings, uint8_t* control_buffer,
    uint16_t control_buffer_size);

/**
 * Register a callback function to be called when the device is reset.
 *
 * @param device USB device
 * @param callback callback function
 */
void qusb_dev_register_reset_callback(qusb_device* device, void (*callback)(void));

/**
 * Register a callback function to be called when the device is suspended.
 *
 * @param device USB device
 * @param callback callback function
 */
void qusb_dev_register_suspend_callback(qusb_device* device, void (*callback)(void));

/**
 * Register a callback function to be called when the device is resumed.
 *
 * @param device USB device
 * @param callback callback function
 */
void qusb_dev_register_resume_callback(qusb_device* device, void (*callback)(void));

/**
 * Register a callback function to be called when the device receives the SOF packet (once every ms).
 *
 * @param device USB device
 * @param callback callback function
 */
void qusb_dev_register_sof_callback(qusb_device* device, void (*callback)(void));

/**
 * Function pointer type for control request completion.
 *
 * A function of this type can be specified when handling a control request.
 * The function will be called when the control request has completed.
 *
 * @param device USB device
 * @param request control request
 */
typedef void (*qusb_dev_control_completion_callback_fn)(qusb_device* device, qusb_setup_data* request);

/**
 * Function pointer type for control request handling.
 *
 * A function of this type can be registered to be called when certain control requests
 * need to be handled.
 *
 * If the control request includes incoming data (from a DATA OUT stage), the
 * data will be contained in the data buffer. `buf` and `len` specify the
 * buffer and its length.
 *
 * If data should be sent as a response to this request (DATA IN stage),
 * `buf` and `len` are used to set the data buffer and data length. `len`
 * indicates the maximum size of the response. If the effective size is smaller,
 * it must be decreased to match it. However, it may not be increased. Initially,
 * `buf` references the control data buffer passed to `qusb_dev_init()`. It can
 * be used to store the data, or the reference can be used to set another buffer.
 *
 * Optionally, a completion function can be set. It will be called when the control
 * request has completed.
 *
 * @param device USB device
 * @param request USB control request
 * @param buf reference to data buffer address (in/out)
 * @param len reference to data buffer length (in/out)
 * @param completion completion function (out, can be `NULL`)
 * @return code indicating if the control request has been handled
 */
typedef qusb_request_return_code (*qusb_dev_control_callback_fn)(qusb_device* device, qusb_setup_data* request, uint8_t** buf,
    uint16_t* len, qusb_dev_control_completion_callback_fn* completion);

/**
 * Function pointer type for device configuration.
 *
 * A function of this type can be registered and will be called when the host
 * sets the device configuration.
 *
 * @param device USB device
 * @param wValue configuration value
 */
typedef void (*qusb_dev_set_config_callback_fn)(qusb_device* device, uint16_t wValue);

/**
 * Function pointer type for interface configuration.
 *
 * A function of this type can be registered and will be called when the host
 * sends a "set interface" (alternate setting) request.
 *
 * @param device USB device
 * @param wIndex interface index
 * @param wValue alternative setting
 */
typedef void (*qusb_dev_set_altsetting_callback_fn)(qusb_device* device, uint16_t wIndex, uint16_t wValue);

/**
 * Function pointer type for endpoint callback.
 *
 * A function of this type is registered when setting up endpoints.
 * It will be called when the endpoint has received data or completed transmitting data.
 *
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param len number of bytes received or transmitted
 */
typedef void (*qusb_dev_ep_callback_fn)(qusb_device* device, uint8_t addr, uint32_t len);

/**
 * Register application callback function for handling USB control requests.
 *
 * The callback function is called for control requests matching the filter.
 * The filter is specified with a type and a type maks. The filter matches
 * if the `bmRequestType` field is equal to `type` after it has been masked with
 * `type_mask`, i.e. if `(bmRequestType & type_mask) == type`.
 *
 * Since the user control callbacks are cleared every time the device configuration is set,
 * control collback registration must happen inside (or after) the configuration callback.
 *
 * @sa qusb_dev_register_set_config_callback
 *
 * @param device USB device
 * @param type request type (set of `QUSB_REQ_TYPE_xxx` constants combined using bitwise or)
 * @param type_mask request type mask (set of `QUSB_REQ_TYPE_xxx_MASK` constants combined using bitwise or)
 * @param callback callback function
 * @return returns 0 on success, -1 if the maximum number of callbacks have already been registered
 */
int qusb_dev_register_control_callback(qusb_device* device, uint8_t type, uint8_t type_mask, qusb_dev_control_callback_fn callback);

/**
 * Register a function to be called when the host sets the configuration.
 *
 * The "set config" callback is the place to setup endpoints.
 *
 * Multiple functions can be registered. They will be called in the order they have been registered.
 *
 * @param device USB device
 * @param callback callback function
 * @return 0 if successful (incl. function is already registered)
 * @return -1 if no more space was available for more callback functions
 */
int qusb_dev_register_set_config_callback(qusb_device* device, qusb_dev_set_config_callback_fn callback);

/**
 * Register a function to be called when the host sends a "set interface" (alternate setting) request.
 *
 * Only a single function can be registered. Any registration replaces the previous registration.
 *
 * @param device USB device
 * @param callback callback function
 */
void qusb_dev_register_set_altsetting_callback(qusb_device* device, qusb_dev_set_altsetting_callback_fn callback);

/**
 * Register a string for the given index.
 *
 * This is useful for a string with a fixed high index that cannot be easily added to the
 * strings array without making it huge.
 *
 * Only a single string can be set. Every call of the function will replace the previous string.
 *
 * @param device USB device
 * @param index string index
 * @param string string
 */
void qusb_dev_register_extra_string(qusb_device* device, int index, const char* string);

/**
 * Poll for USB events.
 *
 * This function handles all pending USB events and calls the registered callback functions.
 *
 * It must be called either from the main loop (at least every 100µs) or from the USB interrupt handler.
 *
 * @param device USB device
 */
void qusb_dev_poll(qusb_device* device);

/**
 * Disconnect the device.
 * 
 * This function is only implemented if the USB peripheral supports it.
 *
 * @param device USB device
 * @param disconnected `true` to request a disconnect, `false` to reconnect
 */
void qusb_dev_disconnect(qusb_device* usbd_dev, bool disconnected);

/**
 * Setup an endpoint.
 * 
 * The endpoint buffer size must be either equal to the maximum packet size
 * specified in the devie descriptor (`wMaxPacketSize`) or a multiple thereof.
 * 
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param type endpoint type (QUSB_ENDPOINT_ATTR_*); it should match `bmAttributes` in the endpoint descriptor
 * @param buffer_size endpoint buffer size
 * @param callback callback function to be called when endpoint has received or transmitted data
 * @note The stack only supports 8 endpoints with addresses 0 to 7 (plus optionally direction bit).
 */
void qusb_dev_ep_setup(
    qusb_device* usbd_dev, uint8_t addr, uint8_t type, uint16_t buffer_size, qusb_dev_ep_callback_fn callback);

/**
 * Get maximum length of data that can be submitted for writing.
 * @param device USB device
 * @return maximum length of data (in bytes)
 */
uint16_t qusb_dev_ep_write_avail(qusb_device* usbd_dev, uint8_t addr);

/**
 * Submit a data packet for transmission.
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param buf pointer to data to be transmitted
 * @param len number of bytes to be transmitted
 * @return 0 if failed, number of bytes submitted if successful
 */
uint16_t qusb_dev_ep_write_packet(qusb_device* usbd_dev, uint8_t addr, const uint8_t* buf, uint16_t len);

/**
 * Retrieve a received data packet.
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param buf buffer that will receive data
 * @param len size of buffer
 * @return Number of bytes written to buffer
 */
uint16_t qusb_dev_ep_read_packet(qusb_device* usbd_dev, uint8_t addr, uint8_t* buf, uint16_t len);

/**
 * Pause the endpoint.
 * 
 * A paused endpoint no longer receives data. Instead, it answers with NAK to all packets.
 * 
 * Due to CPU and USB peripherial working concurrently and due to buffering in the USB
 * peripherial, the endpoint callback might still be called with more data even after
 * the endpoint has been paused. If this function is called from outside the endpoint
 * callback, up to the buffer size (specified when the endpoint was created) of data can
 * still arrive. If this function is called from inside the endpoint callback, up to
 * buffer size minus maximum packet size of data can still arrive. Thus, the only case
 * pausing takes immediate effect is when the buffer size is equal to the maximum packet
 * size and this function is called from within the endpoint callback.
 * 
 * @param device USB device
 * @param addr endpoint address (of an OUT endpoint)
 * @param nak if nonzero, set NAK
 */
void qusb_dev_ep_pause(qusb_device* usbd_dev, uint8_t addr);

/**
 * Unpause the endpoint.
 * 
 * Clears the paused state so the endpoint continues to receive data.
 * 
 * @param device USB device
 * @param addr endpoint address (of an OUT endpoint)
 * @param nak if nonzero, set NAK
 */
void qusb_dev_ep_unpause(qusb_device* usbd_dev, uint8_t addr);

/**
 * Set/clear STALL condition on an endpoint.
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param stall if 0, clear STALL, else set stall.
 */
void qusb_dev_ep_stall_set(qusb_device* usbd_dev, uint8_t addr, uint8_t stall);

/**
 * Get STALL status of an endpoint.
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @return nonzero if endpoint is stalled
 */
uint8_t qusb_dev_ep_stall_get(qusb_device* usbd_dev, uint8_t addr);

#ifdef __cplusplus
}
#endif 
