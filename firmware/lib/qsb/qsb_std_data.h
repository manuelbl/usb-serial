//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Data structure definitions for the USB control requests as
// defined in chapter 9 of the "Universal Serial Bus Specification Revision 2.0".
// Available from the USB Implementers Forum - http://www.usb.org/
//

/**
 * @defgroup qsb_standard USB standard data structure
 */


#pragma once

#include <libopencm3/cm3/common.h>

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @addtogroup qsb_standard
 *
 * @{
 */


/// Number of array elements
#define QSB_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


/**
 * @brief USB setup packet data structure
 * 
 * See Universal Serial Bus Specification Revision 2.0, Table 9-2.
 */
typedef struct qsb_setup_data {
    /// Characteristics of request (see \ref qsb_req_type_e)
    uint8_t bmRequestType;
    /// Specific request (see `QSB_REQ_xxx`)
    uint8_t bRequest;
    /// Word-sized field that varies according to request
    uint16_t wValue;
    /// Word-sized field that varies according to request; typically used to pass an index or offset
    uint16_t wIndex;
    /// Number of bytes to transfer if there is a Data stage
    uint16_t wLength;
} __attribute__((packed)) qsb_setup_data;

// Class Definition

/// USB device class "None" (use class code info from Interface Descriptors)
static const uint8_t QSB_DEV_CLASS_NONE = 0x00;

/// USB device class "Vendor Specific"
static const uint8_t QSB_DEV_CLASS_VENDOR = 0xFF;
/// USB interface class "Miscellaneous"
static const uint8_t QSB_INTF_CLASS_MISCELLANEOUS = 0xEF;
/// USB interface class "Vendor Specific"
static const uint8_t QSB_INTF_CLASS_VENDOR = 0xFF;

// USB Interface Association Descriptor Device Class Code and Use Model (ECN)

/// USB device class "Miscellaneous"
static const uint8_t QSB_DEV_CLASS_MISCELLANEOUS = 0xEF;
/// USB device subclass "Common" (for class "Miscellaneous")
static const uint8_t QSB_DEV_SUBCLASS_MISC_COMMON = 0x02;
/// USB device protocol "Interface association descriptor"
static const uint8_t QSB_DEV_PROTOCOL_INTF_ASSOC_DESC = 0x01;

/// USB request type bit definitions (for `bmRequestType`)
typedef enum {

// bit 7 : direction

    /// `bmRequestType` direction bit mask
    QSB_REQ_TYPE_DIRECTION_MASK = 0x80,
    /// `bmRequestType` direction IN (device-to-host)
    QSB_REQ_TYPE_IN = 0x80,
    /// `bmRequestType` direction OUT (host-to-device)
    QSB_REQ_TYPE_OUT = 0x00,

// bits 6..5 : type

    /// `bmRequestType` request type mask
    QSB_REQ_TYPE_TYPE_MASK = 0x60,
    /// `bmRequestType` request type "standard"
    QSB_REQ_TYPE_STANDARD = 0x00,
    /// `bmRequestType` request type "class"
    QSB_REQ_TYPE_CLASS = 0x20,
    /// `bmRequestType` request type "vendor"
    QSB_REQ_TYPE_VENDOR = 0x40,

// bits 4..0 : recipient

    /// `bmRequestType` request recipient mask
    QSB_REQ_TYPE_RECIPIENT_MASK = 0x1F,
    /// `bmRequestType` request recipient "device"
    QSB_REQ_TYPE_DEVICE = 0x00,
    /// `bmRequestType` request recipient "interface"
    QSB_REQ_TYPE_INTERFACE = 0x01,
    /// `bmRequestType` request recipient "endpoint"
    QSB_REQ_TYPE_ENDPOINT = 0x02,
    /// `bmRequestType` request recipient "other"
    QSB_REQ_TYPE_OTHER = 0x03,

} qsb_req_type_e;

// USB Standard Request Codes - Table 9-4

/// `bRequest` standard request "GET_STATUS"
static const uint8_t QSB_REQ_GET_STATUS = 0;
/// `bRequest` standard request "CLEAR_FEATURE"
static const uint8_t QSB_REQ_CLEAR_FEATURE = 1;
/// `bRequest` standard request "SET_FEATURE"
static const uint8_t QSB_REQ_SET_FEATURE = 3;
/// `bRequest` standard request "SET_ADDRESS"
static const uint8_t QSB_REQ_SET_ADDRESS = 5;
/// `bRequest` standard request "GET_DESCRIPTOR"
static const uint8_t QSB_REQ_GET_DESCRIPTOR = 6;
/// `bRequest` standard request "SET_DESCRIPTOR"
static const uint8_t QSB_REQ_SET_DESCRIPTOR = 7;
/// `bRequest` standard request "GET_CONFIGURATION"
static const uint8_t QSB_REQ_GET_CONFIGURATION = 8;
/// `bRequest` standard request "SET_CONFIGURATION"
static const uint8_t QSB_REQ_SET_CONFIGURATION = 9;
/// `bRequest` standard request "GET_INTERFACE"
static const uint8_t QSB_REQ_GET_INTERFACE = 10;
/// `bRequest` standard request "SET_INTERFACE"
static const uint8_t QSB_REQ_SET_INTERFACE = 11;
/// `bRequest` standard request "SET_SYNCH_FRAME"
static const uint8_t QSB_REQ_SET_SYNCH_FRAME = 12;

// USB Descriptor Types - Table 9-5

/// `bDescriptorType` for device descriptor
static const uint8_t QSB_DT_DEVICE = 1;
/// `bDescriptorType` for configuration descriptor
static const uint8_t QSB_DT_CONFIGURATION = 2;
/// `bDescriptorType` for string descriptor
static const uint8_t QSB_DT_STRING = 3;
/// `bDescriptorType` for interface descriptor
static const uint8_t QSB_DT_INTERFACE = 4;
/// `bDescriptorType` for endpoint descriptor
static const uint8_t QSB_DT_ENDPOINT = 5;
/// `bDescriptorType` for device qualifier descriptor
static const uint8_t QSB_DT_DEVICE_QUALIFIER = 6;
/// `bDescriptorType` for other speed configuration descriptor
static const uint8_t QSB_DT_OTHER_SPEED_CONFIGURATION = 7;
/// `bDescriptorType` for interface power descriptor
static const uint8_t QSB_DT_INTERFACE_POWER = 8;

// From ECNs

/// `bDescriptorType` for OTG descriptor
static const uint8_t QSB_DT_OTG = 9;
/// `bDescriptorType` for DEBUG descriptor
static const uint8_t QSB_DT_DEBUG = 10;
/// `bDescriptorType` for interface association descriptor
static const uint8_t QSB_DT_INTERFACE_ASSOCIATION = 11;

// From USB 3.1/3.2

/// `bDescriptorType` for binary device object store descriptor
static const uint8_t QSB_DT_BOS = 15;
/// `bDescriptorType` for device capability descriptor
static const uint8_t QSB_DT_DEVICE_CAPABILITY = 16;

// USB Standard Feature Selectors - Table 9-6

/// Feature selector for stalling endpoint (used by `SET_FEATURE` and `CLEAR_FEATURE` control requests)
static const uint8_t QSB_FEAT_ENDPOINT_HALT = 0;
/// Feature selector for remote wakeup (used by `SET_FEATURE` and `CLEAR_FEATURE` control requests)
static const uint8_t QSB_FEAT_DEVICE_REMOTE_WAKEUP = 1;
/// Feature selector for test mode (used by `SET_FEATURE` and `CLEAR_FEATURE` control requests)
static const uint8_t QSB_FEAT_TEST_MODE = 2;

// Information Returned by a GetStatus() Request to a Device - Figure 9-4

/// Device status bit value indicating self-powered status (used by `GET_STATUS` control request)
static const uint8_t QSB_DEV_STATUS_SELF_POWERED = 0x01;
/// Device status bit value indicating if the device can request remote wakeup (used by `GET_STATUS` control request)
static const uint8_t QSB_DEV_STATUS_REMOTE_WAKEUP = 0x02;

/**
 * @brief USB endpoint descriptor
 * 
 * See Universal Serial Bus Specification Revision 2.0, Table 9-13.
 * 
 * This structure contains both the endpoint descriptor itself as well as
 * a reference to optional additional descriptors for the endpoint.
 */
typedef struct qsb_endpoint_desc {
    /// Size of this descriptor in bytes (use \ref QSB_DT_ENDPOINT_SIZE)
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_ENDPOINT)
    uint8_t bDescriptorType;
    /// Address of the endpoint on the USB device (incl. direction bit)
    uint8_t bEndpointAddress;
    /// Endpoint attributes bitmap (see \ref qsb_endpoint_attr_e)
    uint8_t bmAttributes;
    /// Maximum packet size this endpoint is capable of sending or receiving
    uint16_t wMaxPacketSize;
    /// Interval for polling endpoint for data transfers
    uint8_t bInterval;

    /// Additional descriptor(s) to be inserted after this descriptor
    const void* extra;
    /// Length of additional descriptor(s) to be inserted after this descriptor
    int extralen;
} __attribute__((packed)) qsb_endpoint_desc;

/// Size of endpoint descriptor
static const uint8_t QSB_DT_ENDPOINT_SIZE = 7;

/**
 * @brief USB interface descriptor
 * 
 * See Universal Serial Bus Specification Revision 2.0, Table 9-12.
 * 
 * This structure contains both the interface descriptor itself as well as
 * a reference to the endpoint descriptors and to optional additional
 * descriptors for the interface.
 */
typedef struct qsb_interface_desc {
    /// Size of this descriptor in bytes (use \ref QSB_DT_INTERFACE_SIZE)
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_INTERFACE)
    uint8_t bDescriptorType;
    /// Number of this interface. Zero-based value identifying the index in the array of concurrent interfaces supported by this configuration.
    uint8_t bInterfaceNumber;
    /// Value used to select this alternate setting for this interface
    uint8_t bAlternateSetting;
    /// Number of endpoints used by this interface (excluding endpoint zero). If this value is zero, this interface only uses the Default Control Pipe.
    uint8_t bNumEndpoints;
    /// Class code (assigned by the USB-IF)
    uint8_t bInterfaceClass;
    /// Subclass code (assigned by the USB-IF)
    uint8_t bInterfaceSubClass;
    /// Protocol code (assigned by the USB)
    uint8_t bInterfaceProtocol;
    /// Index of string descriptor describing this interface
    uint8_t iInterface;
    /// Array of endpoint descriptors. The array must have `bNumEndpoints` entries. This field (pointer) is not transmitted as part of the descriptor.
    const qsb_endpoint_desc* endpoint;
    /// Additional descriptor(s) to be inserted after this descriptor
    const void* extra;
    /// Length of additional descriptor(s) to be inserted after this descriptor
    int extralen;
} __attribute__((packed)) qsb_interface_desc;

/// Size of interface descriptor
static const uint8_t QSB_DT_INTERFACE_SIZE = 9;

/**
 * @brief USB interface association descriptor
 * 
 * See USB ECN re Interface Association Descriptors, Table 9-Z
 */
typedef struct qsb_iface_assoc_desc {
    /// Size of this descriptor in bytes (use \ref QSB_DT_INTERFACE_ASSOCIATION_SIZE)
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_INTERFACE_ASSOCIATION)
    uint8_t bDescriptorType;
    /// Interface number of the first interface that is associated with this function
    uint8_t bFirstInterface;
    /// Number of contiguous interfaces that are associated with this function
    uint8_t bInterfaceCount;
    /// Class code (assigned by USB-IF)
    uint8_t bFunctionClass;
    /// Subclass code (assigned by USB-IF)
    uint8_t bFunctionSubClass;
    /// Subclass code (assigned by USB-IF)
    uint8_t bFunctionProtocol;
    /// Index of string descriptor describing this function
    uint8_t iFunction;
} __attribute__((packed)) qsb_iface_assoc_desc;

/// Size of interface association descriptor
static const uint8_t QSB_DT_INTERFACE_ASSOCIATION_SIZE = sizeof(qsb_iface_assoc_desc);

/**
 * @brief USB interface.
 * 
 * Auxillary data structure for linking the descriptors belonging to a single interface.
 */
typedef struct qsb_interface {
    /**
     * @brief Pointer to variable storing the current alternate interface setting.
     * 
     * If alternate settings are used, it should point to a variable in RAM. The variable
     * will be updated when a `SET_INTERFACE` control request is processed. And it is used
     * to answer `GET_INTERFACE` requests. If it is set to `NULL`, all `GET_INTERFACE`
     * requests will be answer with 0.
     * 
     * If alternate settings are not used, this field should be set to `NULL`.
     */
    uint8_t* cur_altsetting;
    /**
     * @brief Number of alternate settings (including primary) for this interface.
     * If the interface has only a single setting, set to 1.
     */
    uint8_t num_altsetting;
    /// Array of interface descriptors. The array must have `num_altsetting` entries.
    const qsb_interface_desc* altsetting;
    /// Interface association descriptor for this interface. Only set this for the first interface belonging to the association.
    const qsb_iface_assoc_desc* iface_assoc;
} qsb_interface;

/**
 * @brief USB configuration descriptor.
 * 
 * See Universal Serial Bus Specification Revision 2.0, Table 9-10.
 * 
 * This structure contains both the configuration descriptor itself as well as
 * a reference to the interfaces of this configuration.
 */
typedef struct qsb_config_desc {
    /// Size of this descriptor in bytes (use \ref QSB_DT_CONFIGURATION_SIZE)
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_CONFIGURATION)
    uint8_t bDescriptorType;
    /// Total length of data returned for this configuration. Set to 0 as it will be automatically determined at run-time.
    uint16_t wTotalLength;
    /// Number of interfaces supported by this configuration
    uint8_t bNumInterfaces;
    /// Value to use as an argument to the SetConfiguration() request to select this configuration. Typically use 1.
    uint8_t bConfigurationValue;
    /// Index of string descriptor describing this configuration
    uint8_t iConfiguration;
    /// Configuration characteristics bitmap (see \ref qsb_config_attr_e)
    uint8_t bmAttributes;
    /// Maximum power consumption of the USB device from the bus in this specific configuration when the device is fully operational (Expressed in 2 mA units)
    uint8_t bMaxPower;

    /// Array of interfaces. The array must contain `bNumInterfaces`. This field (pointer) is not transmitted as part of the descriptor.
    const qsb_interface* interface;
} __attribute__((packed)) qsb_config_desc;

/// Size of interface descriptor
static const uint8_t QSB_DT_CONFIGURATION_SIZE = 9;

// USB Standard Device Descriptor - Table 9-8

/**
 * @brief USB device descriptor
 * 
 * See Universal Serial Bus Specification Revision 2.0, Table 9-8.
 */
typedef struct qsb_device_desc {
    /// Size of this descriptor in bytes (use \ref QSB_DT_DEVICE_SIZE)
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_DEVICE)
    uint8_t bDescriptorType;
    /**
     * @brief USB Specification Release Number in Binary-Coded Decimal
     * 
     * In most cases, this should be set to 0x0200 (2.0.0). If WebUSB or
     * BOS is used, it should be set to 0x0210 (2.1.0).
     */
    uint16_t bcdUSB;
    /// Class code (assigned by the USB-IF)
    uint8_t bDeviceClass;
    /// Subclass code (assigned by the USB-IF)
    uint8_t bDeviceSubClass;
    /// Protocol code (assigned by the USB-IF)
    uint8_t bDeviceProtocol;
    /// Maximum packet size for endpoint 0 (only 8, 16, 32, or 64 are valid values)
    uint8_t bMaxPacketSize0;
    /// Vendor ID (assigned by the USB-IF)
    uint16_t idVendor;
    /// Product ID (assigned by the manufacturer)
    uint16_t idProduct;
    /// Device release number in binary-coded decimal
    uint16_t bcdDevice;
    /// Index of string descriptor describing manufacturer
    uint8_t iManufacturer;
    /// Index of string descriptor describing product
    uint8_t iProduct;
    /// Index of string descriptor describing product
    uint8_t iSerialNumber;
    /// Number of possible configurations (must match number of entries in configuration array passed to \ref qsb_dev_init())
    uint8_t bNumConfigurations;
} __attribute__((packed)) qsb_device_desc;

/// Size of device descriptor
static const uint8_t QSB_DT_DEVICE_SIZE = sizeof(qsb_device_desc);

// USB Configuration Descriptor bmAttributes bit definitions

/// Configuration attributes bit definitions (for `bmAttributes`)
typedef enum {
    /// `bmAttributes` value for reserved bits that must always be set to 1 (for configuration descriptor)
    QSB_CONFIG_ATTR_DEFAULT = 0x80,
    /// `bmAttributes` value indicating the device is bus-powered (for configuration descriptor)
    QSB_CONFIG_ATTR_SELF_POWERED = 0x40,
    /// `bmAttributes` value indicating the device supports remote wakeup (for configuration descriptor)
    QSB_CONFIG_ATTR_REMOTE_WAKEUP = 0x20,
} qsb_config_attr_e;

// USB bEndpointAddress helper macros

/// Endpoint address bit indicating direction IN (device-to-host)
static const uint8_t QSB_ENDPOINT_ADDR_IN_BIT = 0x80;
/// Endpoint address bit mask for the endpoint number
static const uint8_t  QSB_ENDPOINT_NUM_MASK = 0x0F;

/**
 * @brief Gets endpoint number of endpoint address.
 * 
 * @param addr Endpoint address
 * @return Endpoint number
 */
static __inline__ uint8_t qsb_endpoint_num(uint8_t addr) { return addr & QSB_ENDPOINT_NUM_MASK; }

/**
 * @brief Indicates if direction is device-to-host (IN).
 * 
 * @param addr Endpoint address
 * @return `true` if direction is TX / device-to-host / IN
 * @return `false` if direction is RX / host-to-device / OUT
 */
static __inline__ bool qsb_endpoint_is_tx(uint8_t addr) { return (addr & QSB_ENDPOINT_ADDR_IN_BIT) != 0; }

/**
 * @brief Returns endpoint address in OUT direction (host-to-device).
 * 
 * @param num Endpoint number
 * @return Endpoint address
 */
static __inline__ uint8_t qsb_endpoint_addr_out(uint8_t num) { return num; }

/**
 * @brief Returns endpoint address in IN direction (device-to-host).
 * 
 * @param num Endpoint number
 * @return Endpoint address
 */
static __inline__ uint8_t qsb_endpoint_addr_in(uint8_t num) { return QSB_ENDPOINT_ADDR_IN_BIT | num; }

// USB Endpoint Descriptor bmAttributes bit definitions - Table 9-13

/// Endpoint attribute bit definitions for `bmAttributes`
typedef enum {

// bits 1..0 : transfer type

    /// `bmAttributes` value for transfer type "control" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_CONTROL = 0x00,
    /// `bmAttributes` value for transfer type "isochronous" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_ISOCHRONOUS = 0x01,
    /// `bmAttributes` value for transfer type "bull" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_BULK = 0x02,
    /// `bmAttributes` value for transfer type "interrupt" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_INTERRUPT = 0x03,
    /// `bmAttributes` bit mask for transfer type (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_TRANSFER_TYPE_MASK = 0x03,

// bits 3..2 : Sync type (only if ISOCHRONOUS)

    /// `bmAttributes` value for synchronization type "no sychronization" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_NOSYNC = 0x00,
    /// `bmAttributes` value for synchronization type "asynchronous" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_ASYNC = 0x04,
    /// `bmAttributes` value for synchronization type "adaptive" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_ADAPTIVE = 0x08,
    /// `bmAttributes` value for synchronization type "synchronous" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_SYNC = 0x0C,
    /// `bmAttributes` bit mask for synchronization type (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_SYNC_TYPE_MASK = 0x0C,

// bits 5..4 : usage type (only if ISOCHRONOUS)

    /// `bmAttributes` value for usage type "data endpoint" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_DATA = 0x00,
    /// `bmAttributes` value for usage type "feedback endpoint" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_FEEDBACK = 0x10,
    /// `bmAttributes` value for usage type "implicit feedback data endpoint" (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_IMPLICIT_FEEDBACK_DATA = 0x20,
    /// `bmAttributes` bit mask for usage type (for endpoint descriptor)
    QSB_ENDPOINT_ATTR_USAGE_TYPE_MASK = 0x30,

} qsb_endpoint_attr_e;

// Table 9-15 specifies String Descriptor Zero
// Table 9-16 specified UNICODE String Descriptor

/**
 * @brief USB string descriptor.
 */
typedef struct qsb_string_desc {
    /// Size of this descriptor in bytes
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_STRING)
    uint8_t bDescriptorType;
    /// UTF-16 LE encoded string (not null terminated)
    uint16_t wData[];
} __attribute__((packed)) qsb_string_desc;

/// Language ID for English (US)
static const uint16_t QSB_LANGID_ENGLISH_US = 0x409;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif 
