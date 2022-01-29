//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Private declaration for the Qusb device implementation
//

#pragma once

#include "qusb_config.h"
#include "qusb_device.h"

#if QUSB_ARCH == QUSB_ARCH_DWC
#include "qusb_otg_common.h"
#define MAX_NUM_ENDPOINTS 4
#else
#define MAX_NUM_ENDPOINTS 8
#endif

#define MAX_USER_CONTROL_CALLBACK 4
#define MAX_USER_SET_CONFIG_CALLBACK 4

/**
 * Return the minimum of the given arguments.
 *
 * @param a argument a
 * @param b argument b
 * @return minimum
 */
static inline uint16_t min_u16(uint16_t a, uint16_t b)
{
    return a <= b ? a : b;
}

/**
 * Return the minimum of the given arguments.
 *
 * @param a argument a
 * @param b argument b
 * @return minimum
 */
static inline uint32_t min_u32(uint32_t a, uint32_t b)
{
    return a <= b ? a : b;
}

/** Internal collection of device information. */
struct _qusb_device {
#if QUSB_ARCH == QUSB_ARCH_DWC
    qusb_otg* otg;
#endif

    // -- descriptor data structures
    const qusb_device_desc* desc;
    const qusb_config_desc* config;
    const char* const* strings;
    int num_strings;

    /// Internal buffer used for control transfers
    uint8_t* ctrl_buf;
    /// Length of internal buffer used for control transfers
    uint16_t ctrl_buf_len;

    /// Currently selected configuration
    uint8_t current_config;

    /// Endpoint address (incl. direction bit) whose callback is currently called
    uint8_t active_ep_callback;

    // User callback functions for various USB events
    void (*user_callback_reset)(void);
    void (*user_callback_suspend)(void);
    void (*user_callback_resume)(void);
    void (*user_callback_sof)(void);

    struct {
        enum {
            IDLE,
            STALLED,
            DATA_IN,
            LAST_DATA_IN,
            STATUS_IN,
            DATA_OUT,
            LAST_DATA_OUT,
            STATUS_OUT,
        } state;
        qusb_setup_data req __attribute__((aligned(4)));
        uint8_t* ctrl_buf;
        uint16_t ctrl_len;
        qusb_dev_control_completion_callback_fn completion;
    } control_state;

    struct user_control_callback {
        qusb_dev_control_callback_fn cb;
        uint8_t type;
        uint8_t type_mask;
    } user_control_callback[MAX_USER_CONTROL_CALLBACK];

    qusb_dev_ep_callback_fn ep_callbacks[MAX_NUM_ENDPOINTS][3];

    // User callback function for some standard USB function hooks
    qusb_dev_set_config_callback_fn user_callback_set_config[MAX_USER_SET_CONFIG_CALLBACK];

    qusb_dev_set_altsetting_callback_fn user_callback_set_altsetting;

    // Extra, non-contiguous user string descriptor index and value
    int extra_string_idx;
    const char* extra_string;

#if QUSB_ARCH == QUSB_ARCH_FSDEV

    // private implementation data for USB full-speed device peripheral

    uint16_t pm_top; // Top of allocated endpoint buffer memory
    uint8_t ep_state_rx[MAX_NUM_ENDPOINTS];
    uint8_t ep_state_tx[MAX_NUM_ENDPOINTS];

#elif QUSB_ARCH == QUSB_ARCH_DWC

    // private implementation data for the DesignWare USB core

    uint16_t rx_fifo_size;
    uint16_t fifo_mem_top;
    uint16_t fifo_mem_top_ep0;
    uint8_t force_nak[MAX_NUM_ENDPOINTS];

    /// Initial state of OUT endpoint size register (basically maximum packet size)
    uint32_t doeptsiz[MAX_NUM_ENDPOINTS];

    /// Number of bytes submitted for transmission
    uint16_t tx_pkt_len[MAX_NUM_ENDPOINTS];

    /// Number of bytes of next packet in RX FIFO
    uint16_t rx_pkt_len;

#endif
};

enum _qusb_dev_transaction { QUSB_TRANSACTION_IN = 0, QUSB_TRANSACTION_OUT = 1, QUSB_TRANSACTION_SETUP = 2 };

// --- Private functions for control endpoint ---

/**
 * Handle CONTROL IN events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void _qusb_control_in(qusb_device* device, uint8_t ep, uint32_t len);

/**
 * Handle CONTROL OUT events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void _qusb_control_out(qusb_device* device, uint8_t ep, uint32_t len);

/**
 * Handle SETUP events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void _qusb_control_setup(qusb_device* device, uint8_t ep, uint32_t len);

/**
 * Handle standard control requests for devices.
 *
 * See `_qusb_standard_request()` for a documentation of the parameters.
 */
qusb_request_return_code _qusb_standard_request_device(
    qusb_device* usbd_dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests for interfaces.
 *
 * See `_qusb_standard_request()` for a documentation of the parameters.
 */
qusb_request_return_code _qusb_standard_request_interface(
    qusb_device* usbd_dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests for endpoints.
 *
 * See `_qusb_standard_request()` for a documentation of the parameters.
 */
qusb_request_return_code _qusb_standard_request_endpoint(
    qusb_device* usbd_dev, qusb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests.
 *
 * If the control request includes incoming data (from a DATA OUT stage), the
 * data will be contained in the data buffer. `buf` and `len` specify the
 * buffer and its length.
 *
 * If data should be sent as a response to this request (DATA IN stage),
 * `buf` and `len` are used to set the data buffer and data length. `len`
 * indicates the maximum size of the response. If needed, it should be
 * decreased to match the effective size of the response. However, it may
 * not increased. Initially, `buf` references the control data buffer passed
 * to the qusb_dev_init() call. It can be used to store the data, or the
 * reference can be used to set another buffer.
 *
 * @param dev USB device
 * @param req USB control request
 * @param buf in/out reference to data buffer address
 * @param len in/out reference to data buffer length
 */
qusb_request_return_code _qusb_standard_request(qusb_device* device, qusb_setup_data* req, uint8_t** buf, uint16_t* len);

void _qusb_dev_reset(qusb_device* usbd_dev);

void _qusb_ep_reset(qusb_device* device);

void _qusb_dev_set_address(qusb_device* device, uint8_t addr);

#if defined(QUSB_ARCH_FSDEV)
typedef enum { qusb_offset_tx = 0, qusb_offset_db0 = 0, qusb_offset_rx = 1, qusb_offset_db1 = 1 } qusb_buf_desc_offset;
#endif
