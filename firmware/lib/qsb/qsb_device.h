//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Main header file to be included when using the QSB library
//

/**
 * @defgroup qsb_device USB device
 */

#pragma once

#include "qsb_config.h"
#include "qsb_std_data.h"
#include "qsb_bos.h"
#include "qsb_windows.h"

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @addtogroup qsb_device
 *
 * @{
 */


/// Return codes for functions handling control requests
typedef enum qsb_request_return_code {
    /// Request not supported (cancel request handling)
    QSB_REQ_NOTSUPP = 0,
    /// Request handled
    QSB_REQ_HANDLED = 1,
    /// Request not handled, pass on the next handler
    QSB_REQ_NEXT_HANDLER = 2,
} qsb_request_return_code;

/// USB device (opaque data structure)
typedef struct qsb_internal_device qsb_device;

/// USB port (creator function)
typedef qsb_device* (*qsb_port)(void);


/// USB full-speed port (PA12 and PA11)
extern qsb_port qsb_port_fs;

#if QSB_ARCH == QSB_ARCH_DWC
/// USB high-speed port (TODO)
extern qsb_port qsb_port_hs;
#endif

/**
 * @brief Initializes USB device.
 *
 * This function assumes that the USB clock has already been configured
 * for USB operation (usually for 48MHz).
 *
 * To place `strings` entirely into Flash/read-only memory, use
 * `static const * const strings[] = { ... };`
 * (note the double `const`).  The first `const` refers to the strings
 * while the second `const` refers to the array.
 * 
 * If `QSB_STR_UTF8` is not defined, the strings in the `strings` array have to be
 * encoded in Latin-1 (aka ISO-8859-1). If `QSB_STR_UTF8` is defined, UTF-8 encoding
 * is assumed. Note that USB 2.0 refers to Unicode version 4.0 (in an ECN) and
 * USB 3.2 refers to Unicode version 5.0 while the latest Unicode is version 14.0.
 * So the latest Unicode characters such as emojis might work but are not officially
 * supported by the standard.
 *
 * @param port USB port
 * @param device_desc USB device descriptor. This must not be changed while the device is in use.
 * @param config_descs Array of USB configuration descriptors. These must not be changed while the device is in use. The
 * length of this array is determined by the `bNumConfigurations` field in the device descriptor.
 * @param strings Array of strings for USB string descriptors. The device, configuration and interface descriptors refer
 * to these strings, e.g. in `iManufacturer`. Strings indexes are offset by 1: index 1 refers to `strings[0]`.
 * @param num_strings Number of items in `strings` array.
 * @param control_buffer Array that will hold the data received during control requests with DATA stage. The array must
 * be suffuciently large to hold the longest configuration descriptor and the longest in-bound control message.
 * @param control_buffer_size Size of `control_buffer`
 * @return the USB device initialized for use
 */
qsb_device* qsb_dev_init(qsb_port port, const qsb_device_desc* device_desc,
    const qsb_config_desc* config_descs, const char* const* strings, int num_strings, uint8_t* control_buffer,
    uint16_t control_buffer_size);

#if QSB_BOS == 1

/**
 * @brief Initializes the binary device object store (BOS).
 * 
 * If this function has been called, the device will respond to *GetDescriptor* requests
 * related to the binary device object store.
 * 
 * Device capability descriptors have different subtypes and each subtype can have a different
 * length. The actual length is given in the descriptor header.
 * 
 * In order to use this function, BOS handling must be enabled by defining the macro `QSB_BOS_ENABLE`.
 * 
 * @param device USB device
 * @param descs array of device capability descriptors
 * @param num_descs number of descriptors in array
 */
void qsb_dev_init_bos(qsb_device* device, const qsb_bos_device_capability_desc* const* descs, int num_descs);

#endif

/**
 * @brief Registers a callback function to be called when the device is reset.
 * 
 * A single function can be registered. A call to this function overwrites the
 * previous callback registration. To unregister the callback, pass `NULL` as
 * the `callback` argument.
 *
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_reset_callback(qsb_device* device, void (*callback)(void));

/**
 * @brief Registers a callback function to be called when the device is suspended.
 * 
 * A single function can be registered. A call to this function overwrites the
 * previous callback registration. To unregister the callback, pass `NULL` as
 * the `callback` argument.
 *
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_suspend_callback(qsb_device* device, void (*callback)(void));

/**
 * @brief Registers a callback function to be called when the device is resumed.
 * 
 * A single function can be registered. A call to this function overwrites the
 * previous callback registration. To unregister the callback, pass `NULL` as
 * the `callback` argument.
 *
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_resume_callback(qsb_device* device, void (*callback)(void));

/**
 * @brief Registers a callback function to be called when the device receives the SOF packet (once every ms).
 * 
 * A single function can be registered. A call to this function overwrites the
 * previous callback registration. To unregister the callback, pass `NULL` as
 * the `callback` argument.
 *
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_sof_callback(qsb_device* device, void (*callback)(void));

/**
 * @brief Function pointer type for control request completion.
 *
 * A function of this type can be specified when handling a control request.
 * The function will be called when the control request has completed, i.e.
 * the request response has been successfully transmitted.
 *
 * @param device USB device
 * @param request control request
 */
typedef void (*qsb_dev_control_completion_callback_fn)(qsb_device* device, qsb_setup_data* request);

/**
 * @brief Function pointer type for control request handling.
 *
 * A function of this type can be registered to be called when certain control requests
 * need to be handled.
 *
 * If the control request includes incoming data (from a DATA OUT stage), the
 * data will be contained in the data buffer. `buf` and `len` specify the
 * buffer and its length.
 *
 * If data should be sent as a response to this request (DATA IN stage),
 * `buf` and `len` are used to set the data buffer and data length. `len` initially
 * indicates the maximum size of the response. If the effective size is smaller,
 * it must be decreased to match it. However, it may not be increased. Initially,
 * `buf` references the control data buffer passed to `qsb_dev_init()`. It can
 * be used to store the data, or it can be modified to point to another buffer.
 *
 * Optionally, a completion function can be set. It will be called when the control
 * request has completed, i.e. the response has been successfully transmitted.
 *
 * @param device USB device
 * @param request USB control request
 * @param buf reference to data buffer address (in/out)
 * @param len reference to data buffer length (in/out)
 * @param completion completion function (out, can be `NULL`)
 * @return code indicating if the control request has been handled
 */
typedef qsb_request_return_code (*qsb_dev_control_callback_fn)(qsb_device* device, qsb_setup_data* request, uint8_t** buf,
    uint16_t* len, qsb_dev_control_completion_callback_fn* completion);

/**
 * @brief Function pointer type for device configuration.
 *
 * A function of this type can be registered and will be called when the host
 * sets the device configuration.
 *
 * @param device USB device
 * @param wValue configuration value
 */
typedef void (*qsb_dev_set_config_callback_fn)(qsb_device* device, uint16_t wValue);

/**
 * @brief Function pointer type for interface configuration.
 *
 * A function of this type can be registered and will be called when the host
 * sends a "set interface" (alternate setting) request.
 *
 * @param device USB device
 * @param wIndex interface index
 * @param wValue alternate setting
 */
typedef void (*qsb_dev_set_altsetting_callback_fn)(qsb_device* device, uint16_t wIndex, uint16_t wValue);

/**
 * @brief Function pointer type for endpoint callback.
 *
 * A function of this type is registered when setting up endpoints.
 * It will be called when the endpoint has received data or completed transmitting data.
 *
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param len number of bytes received or transmitted
 */
typedef void (*qsb_dev_ep_callback_fn)(qsb_device* device, uint8_t addr, uint32_t len);

/**
 * @brief Registers callback function for handling USB control requests.
 *
 * The callback function is called for control requests matching the filter.
 * The filter is specified with a type and a type mask. The filter matches
 * if the `bmRequestType` field is equal to `type` after it has been masked with
 * `type_mask`, i.e. if `(bmRequestType & type_mask) == type`.
 *
 * Since control callbacks are cleared every time the device configuration is set,
 * control callback are usually registered in the configuration callback.
 * 
 * Control requests directed to the device (instead of to the interface or endpoint) likely
 * occur before the configuration is set. So the callback must be registered after the USB 
 * device is initialized. Since they are also cleared when the device configuration is set, 
 * they need to be registered in the configuration callback if they are still needed.
 * 
 * Multiple functions can be registered. They will be called in the order they have been registered.
 * 
 * The number of callbacks that can be registered is limited. By default, it is 4. If this
 * is insufficient, the macro `QSB_MAX_CONTROL_CALLBACKS` can be set to a higher number.
 * 
 * Registered control callbacks have priority over the standdard control request handling.
 * So the standard implementation can be selectively overridden.
 *
 * @sa qsb_dev_register_set_config_callback
 *
 * @param device USB device
 * @param type request type (set of \ref qsb_req_type_e constants combined using bitwise or)
 * @param type_mask request type mask (set of `QSB_REQ_TYPE_xxx_MASK` (see \ref qsb_req_type_e) constants combined using bitwise or)
 * @param callback callback function
 */
void qsb_dev_register_control_callback(qsb_device* device, uint8_t type, uint8_t type_mask, qsb_dev_control_callback_fn callback);

/**
 * @brief Registers a function to be called when the host sets the configuration.
 *
 * The configuration callback is the place to setup endpoints.
 *
 * Multiple functions can be registered. They will be called in the order they have been registered.
 *
 * The number of callbacks that can be registered is limited. By default, it is 4. If this
 * is insufficient, the macro `QSB_MAX_SET_CONFIG_CALLBACKS` can be set to a higher number.
 * 
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_set_config_callback(qsb_device* device, qsb_dev_set_config_callback_fn callback);

/**
 * @brief Registers a function to be called when the host sends a "set interface" (alternate setting) request.
 *
 * Only a single function can be registered. Any registration replaces the previous registration.
 *
 * @param device USB device
 * @param callback callback function
 */
void qsb_dev_register_set_altsetting_callback(qsb_device* device, qsb_dev_set_altsetting_callback_fn callback);

/**
 * @brief Polls for USB events.
 *
 * This function handles all pending USB events and calls the registered callback functions.
 *
 * It must be called either from the main loop (at least every 100µs) or from the USB interrupt handler.
 *
 * @param device USB device
 */
void qsb_dev_poll(qsb_device* device);

/**
 * @brief Disconnects the device.
 * 
 * This function is only implemented if the USB peripheral supports it.
 *
 * @param device USB device
 * @param disconnected `true` to request a disconnect, `false` to reconnect
 */
void qsb_dev_disconnect(qsb_device* device, bool disconnected);

/**
 * @brief Sets up an endpoint.
 * 
 * The endpoint buffer size must be either equal to the maximum packet size
 * specified in the devie descriptor (`wMaxPacketSize`) or a multiple thereof.
 * Valid values for `wMaxPacketSize` are 8, 16, 32 or 64 (bytes) for full-speed
 * endpoints.
 * 
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param type endpoint type (`QSB_ENDPOINT_ATTR_*`); it should match `bmAttributes` in the endpoint descriptor
 * @param buffer_size endpoint buffer size
 * @param callback callback function to be called when endpoint has received or transmitted data
 * @note This library only supports 8 endpoints with addresses 0 to 7 (plus optionally the direction bit).
 */
void qsb_dev_ep_setup(
    qsb_device* device, uint8_t addr, uint32_t type, int buffer_size, qsb_dev_ep_callback_fn callback);

/**
 * @brief Gets maximum length of data that can be submitted for transmission.
 * 
 * If 0 is returned, no packet can be submitted for transmission, not even
 * a zero length packet.
 * 
 * @param device USB device
 * @param addr endpoint address incl. direction bit (of an IN endpoint)
 * @return maximum length of data (in bytes)
 */
uint16_t qsb_dev_ep_transmit_avail(qsb_device* device, uint8_t addr);

/**
 * @brief Submits a data packet for transmission.
 * 
 * The specified data buffer can immediately be reused as the data is copied
 * by the function.
 * 
 * Once the data has been transmitted, the endpoint callback function is called.
 * 
 * @param device USB device
 * @param addr endpoint address incl. direction bit (of an IN endpoint)
 * @param buf pointer to data to be transmitted
 * @param len number of bytes to be transmitted
 * @return -1 if failed, number of bytes submitted if successful
 */
int qsb_dev_ep_transmit_packet(qsb_device* device, uint8_t addr, const uint8_t* buf, int len);

/**
 * @brief Retrieves a received data packet.
 * 
 * This function may only be called from within the endpoint callback function of an OUT endpoint.
 * As the callback is called for each packet, the maximum available data size is the
 * maximum packet size as specified in the endpoint descriptor. The entire packet must be
 * read at once. Remaining data is lost.
 * 
 * @param device USB device
 * @param addr endpoint address (of an OUT endpoint)
 * @param buf buffer that will receive data
 * @param len size of buffer
 * @return Number of bytes written to buffer
 */
uint16_t qsb_dev_ep_read_packet(qsb_device* device, uint8_t addr, uint8_t* buf, uint16_t len);

/**
 * @brief Pauses the endpoint.
 * 
 * A paused endpoint no longer receives data. Instead, it answers with NAK to all packets.
 * This function is mainly used to implement flow control for incoming (OUT) bulk endpoint.
 * Pausing the endpoint is the signal for the host to hold back data until the endpoint
 * is unpaused.
 * 
 * If the buffer size specified when the endpoint was setup is equal to the maximum packet
 * size, pausing the endpoint will take immediate effect if it is called from within the
 * callback function. No further callbacks will be received until the endpoint is unpaused.
 * 
 * If a bigger buffer has been specified or the function is called from outside the callback,
 * further callbacks may arrive until the buffer is empty. If this function is called from
 * outside the endpoint callback, up to the buffer size of data can still arrive. If this
 * function is called from inside the endpoint callback, up to the buffer size minus the
 * maximum packet size of data can still arrive.
 * 
 * @param device USB device
 * @param addr endpoint address (of an OUT endpoint)
 */
void qsb_dev_ep_pause(qsb_device* device, uint8_t addr);

/**
 * @brief Unpauses the endpoint.
 * 
 * Clears the paused state so the endpoint continues to receive data.
 * 
 * @sa qsb_dev_ep_pause
 * 
 * @param device USB device
 * @param addr endpoint address (of an OUT endpoint)
 */
void qsb_dev_ep_unpause(qsb_device* device, uint8_t addr);

/**
 * @brief Sets/clears STALL condition on an endpoint.
 * 
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @param stall if 0, clear STALL, else set stall.
 */
void qsb_dev_ep_stall_set(qsb_device* device, uint8_t addr, uint8_t stall);

/**
 * @brief Gets STALL status of an endpoint.
 * 
 * @param device USB device
 * @param addr endpoint address including direction (e.g. 0x01 or 0x81)
 * @return nonzero if endpoint is stalled
 */
uint8_t qsb_dev_ep_stall_get(qsb_device* device, uint8_t addr);

/**
 * @brief Serial number derived from unique device ID.
 * 
 * This global variable contains the serial number. It can be directly used in the
 * strings table passed  to `qsb_dev_init()`. `qsb_serial_num_init()` must be called
 * to initialize it.
 * 
 * The serial number consists of 8 ASCII characters (plus a terminating null byte).
 * 
 * @sa qsb_serial_num_init
 */
extern char qsb_serial_num[9];

/**
 * @brief Initializes the serial number dervied from unique device ID.
 * 
 * The serial number consists of 8 ASCII characters (plus a terminating null byte).
 * 
 * The serial number is dervied fromm the Unique Device ID that is programmed at the
 * chip factory. For some processor families, the macro QSB_UID_BASE must be set
 * to the base address of the unique device ID as the base address varies across
 * the family.
 * 
 * @return serial number 
 */
const char* qsb_serial_num_init(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif 
