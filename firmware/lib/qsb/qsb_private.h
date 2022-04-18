//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Private declaration for the QSB device implementation
//

#pragma once

#include "qsb_config.h"
#include "qsb_device.h"

#if QSB_ARCH == QSB_ARCH_DWC
#include "qsb_otg_common.h"
#define QSB_NUM_ENDPOINTS 4
#else
#define QSB_NUM_ENDPOINTS 8
#endif

#define QSB_MAX_CONTROL_CALLBACKS 4
#define QSB_MAX_SET_CONFIG_CALLBACKS 4

/**
 * Return the minimum of the given arguments.
 *
 * @param a argument a
 * @param b argument b
 * @return minimum
 */
static inline uint32_t imin(uint32_t a, uint32_t b)
{
    return a <= b ? a : b;
}

/** Internal collection of device information. */
struct qsb_internal_device {
#if QSB_ARCH == QSB_ARCH_DWC
    qsb_otg* otg;
#endif

    // -- descriptor data structures
    const qsb_device_desc* desc;
    const qsb_config_desc* config;
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
        qsb_setup_data req __attribute__((aligned(4)));
        uint8_t* ctrl_buf;
        uint16_t ctrl_len;
        qsb_dev_control_completion_callback_fn completion;
    } control_state;

    struct user_control_callback {
        qsb_dev_control_callback_fn cb;
        uint8_t type;
        uint8_t type_mask;
    } user_control_callback[QSB_MAX_CONTROL_CALLBACKS];

    qsb_dev_ep_callback_fn ep_callbacks[QSB_NUM_ENDPOINTS][3];

    // User callback function for some standard USB function hooks
    qsb_dev_set_config_callback_fn user_callback_set_config[QSB_MAX_SET_CONFIG_CALLBACKS];

    qsb_dev_set_altsetting_callback_fn user_callback_set_altsetting;

#if QSB_BOS == 1
    // Device capability descriptors of binary device object store (BOS)
    int num_bos_descs;
    const qsb_bos_device_capability_desc* const* bos_descs;
#endif

#if QSB_ARCH == QSB_ARCH_FSDEV

    // private implementation data for USB full-speed device peripheral

    uint16_t pm_top; // Top of allocated endpoint buffer memory
    uint8_t ep_state_rx[QSB_NUM_ENDPOINTS];
    uint8_t ep_state_tx[QSB_NUM_ENDPOINTS];

#if defined(QSB_FSDEV_DBL_BUF)
    uint8_t ep_outstanig_rx_acks[QSB_NUM_ENDPOINTS];
#endif

#elif QSB_ARCH == QSB_ARCH_DWC

    // private implementation data for the DesignWare USB core

    uint16_t rx_fifo_size;
    uint16_t fifo_mem_top;
    uint16_t fifo_mem_top_ep0;
    uint8_t force_nak[QSB_NUM_ENDPOINTS];

    /// Initial state of OUT endpoint size register (basically maximum packet size)
    uint32_t doeptsiz[QSB_NUM_ENDPOINTS];
    /// Buffer size of IN endpoints
    uint32_t tx_buf_size[QSB_NUM_ENDPOINTS];

    /// Number of bytes submitted for transmission
    uint16_t tx_pkt_len[QSB_NUM_ENDPOINTS];

    /// Number of bytes of next packet in RX FIFO
    uint16_t rx_pkt_len;

#endif
};

typedef enum { QSB_TRANSACTION_IN = 0, QSB_TRANSACTION_OUT = 1, QSB_TRANSACTION_SETUP = 2 } qsb_internal_dev_transaction;

// --- Private functions for control endpoint ---

/**
 * Handle CONTROL IN events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void qsb_internal_control_in(qsb_device* device, uint8_t ep, uint32_t len);

/**
 * Handle CONTROL OUT events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void qsb_internal_control_out(qsb_device* device, uint8_t ep, uint32_t len);

/**
 * Handle SETUP events of endpoint 0
 *
 * @param dev USB device
 * @param ep endpoint (unused)
 */
void qsb_internal_control_setup(qsb_device* device, uint8_t ep, uint32_t len);

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
 * to the qsb_dev_init() call. It can be used to store the data, or the
 * reference can be used to set another buffer.
 *
 * @param dev USB device
 * @param req USB control request
 * @param buf in/out reference to data buffer address
 * @param len in/out reference to data buffer length
 */
qsb_request_return_code qsb_internal_standard_request(qsb_device* device, qsb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests for devices.
 *
 * See `qsb_internal_standard_request()` for a documentation of the parameters.
 */
qsb_request_return_code qsb_internal_standard_request_device(
    qsb_device* usbd_dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests for interfaces.
 *
 * See `qsb_internal_standard_request()` for a documentation of the parameters.
 */
qsb_request_return_code qsb_internal_standard_request_interface(
    qsb_device* usbd_dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len);

/**
 * Handle standard control requests for endpoints.
 *
 * See `qsb_internal_standard_request()` for a documentation of the parameters.
 */
qsb_request_return_code qsb_internal_standard_request_endpoint(
    qsb_device* usbd_dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len);

#if QSB_BOS == 1

/**
 * Handle device control requests for BOS descriptors.
 *
 * See `qsb_internal_standard_request()` for a documentation of the parameters.
 */
qsb_request_return_code qsb_internal_bos_request_get_desc(
    qsb_device* usbd_dev, qsb_setup_data* req, uint8_t** buf, uint16_t* len);

#endif

#if QSB_WIN_WCID == 1

/// Handle device control requests for MSFT string descriptor.
qsb_request_return_code qsb_internal_win_get_msft_string_desc(uint8_t** buf, uint16_t* len);

/// Handle vendor control requests for WCID
qsb_request_return_code qsb_internal_win_wcid_vendor_request(qsb_setup_data *req, uint8_t **buf, uint16_t *len);


#endif


void qsb_internal_dev_reset(qsb_device* usbd_dev);

void qsb_internal_ep_reset(qsb_device* device);

void qsb_internal_dev_set_address(qsb_device* device, uint8_t addr);

#if defined(QSB_ARCH_FSDEV)
typedef enum { qsb_offset_tx = 0, qsb_offset_db0 = 0, qsb_offset_rx = 1, qsb_offset_db1 = 1 } qsb_buf_desc_offset;
#endif
