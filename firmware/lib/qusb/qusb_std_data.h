//
// Qusb USB Device Library for libopencm3
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

#pragma once

#include <libopencm3/cm3/common.h>

// USB Setup Data structure - Table 9-2
typedef struct qusb_setup_data {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) qusb_setup_data;

// Class Definition
#define QUSB_CLASS_VENDOR 0xFF

// bmRequestType bit definitions
// bit 7 : direction
#define QUSB_REQ_TYPE_DIRECTION_MASK 0x80
#define QUSB_REQ_TYPE_IN 0x80
#define QUSB_REQ_TYPE_OUT 0x00
// bits 6..5 : type
#define QUSB_REQ_TYPE_TYPE_MASK 0x60
#define QUSB_REQ_TYPE_STANDARD 0x00
#define QUSB_REQ_TYPE_CLASS 0x20
#define QUSB_REQ_TYPE_VENDOR 0x40
// bits 4..0 : recipient
#define QUSB_REQ_TYPE_RECIPIENT_MASK 0x1F
#define QUSB_REQ_TYPE_DEVICE 0x00
#define QUSB_REQ_TYPE_INTERFACE 0x01
#define QUSB_REQ_TYPE_ENDPOINT 0x02
#define QUSB_REQ_TYPE_OTHER 0x03

// USB Standard Request Codes - Table 9-4
#define QUSB_REQ_GET_STATUS 0
#define QUSB_REQ_CLEAR_FEATURE 1
// Reserved for future use: 2
#define QUSB_REQ_SET_FEATURE 3
// Reserved for future use: 4
#define QUSB_REQ_SET_ADDRESS 5
#define QUSB_REQ_GET_DESCRIPTOR 6
#define QUSB_REQ_SET_DESCRIPTOR 7
#define QUSB_REQ_GET_CONFIGURATION 8
#define QUSB_REQ_SET_CONFIGURATION 9
#define QUSB_REQ_GET_INTERFACE 10
#define QUSB_REQ_SET_INTERFACE 11
#define QUSB_REQ_SET_SYNCH_FRAME 12

// USB Descriptor Types - Table 9-5
#define QUSB_DT_DEVICE 1
#define QUSB_DT_CONFIGURATION 2
#define QUSB_DT_STRING 3
#define QUSB_DT_INTERFACE 4
#define QUSB_DT_ENDPOINT 5
#define QUSB_DT_DEVICE_QUALIFIER 6
#define QUSB_DT_OTHER_SPEED_CONFIGURATION 7
#define QUSB_DT_INTERFACE_POWER 8
// From ECNs
#define QUSB_DT_OTG 9
#define QUSB_DT_DEBUG 10
#define QUSB_DT_INTERFACE_ASSOCIATION 11

// USB Standard Feature Selectors - Table 9-6
#define QUSB_FEAT_ENDPOINT_HALT 0
#define QUSB_FEAT_DEVICE_REMOTE_WAKEUP 1
#define QUSB_FEAT_TEST_MODE 2

// Information Returned by a GetStatus() Request to a Device - Figure 9-4
#define QUSB_DEV_STATUS_SELF_POWERED 0x01
#define QUSB_DEV_STATUS_REMOTE_WAKEUP 0x02

// USB Standard Endpoint Descriptor - Table 9-13
typedef struct qusb_endpoint_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;

    // Descriptor ends here.  The following are used internally:
    const void* extra;
    int extralen;
} __attribute__((packed)) qusb_endpoint_desc;

#define QUSB_DT_ENDPOINT_SIZE 7

// USB Standard Interface Descriptor - Table 9-12
typedef struct qusb_interface_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;

    // Descriptor ends here. The following are used internally:
    const qusb_endpoint_desc* endpoint;
    const void* extra;
    int extralen;
} __attribute__((packed)) qusb_interface_desc;

#define QUSB_DT_INTERFACE_SIZE 9

// From ECN: Interface Association Descriptors, Table 9-Z
typedef struct qusb_iface_assoc_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bFirstInterface;
    uint8_t bInterfaceCount;
    uint8_t bFunctionClass;
    uint8_t bFunctionSubClass;
    uint8_t bFunctionProtocol;
    uint8_t iFunction;
} __attribute__((packed)) qusb_iface_assoc_desc;

#define QUSB_DT_INTERFACE_ASSOCIATION_SIZE sizeof(qusb_iface_assoc_desc)

typedef struct qusb_interface {
    uint8_t* cur_altsetting;
    uint8_t num_altsetting;
    const qusb_iface_assoc_desc* iface_assoc;
    const qusb_interface_desc* altsetting;
} qusb_interface;

// USB Standard Configuration Descriptor - Table 9-10
typedef struct qusb_config_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;

    // Descriptor ends here. The following are used internally;
    const qusb_interface* interface;
} __attribute__((packed)) qusb_config_desc;
#define QUSB_DT_CONFIGURATION_SIZE 9

// USB Standard Device Descriptor - Table 9-8
typedef struct qusb_device_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) qusb_device_desc;

#define QUSB_DT_DEVICE_SIZE sizeof(qusb_device_desc)

// USB Configuration Descriptor bmAttributes bit definitions
#define QUSB_CONFIG_ATTR_DEFAULT 0x80
#define QUSB_CONFIG_ATTR_SELF_POWERED 0x40
#define QUSB_CONFIG_ATTR_REMOTE_WAKEUP 0x20

// USB bEndpointAddress helper macros
#define QUSB_ENDPOINT_ADDR_IN_BIT 0x80
#define QUSB_ENDPOINT_NUM_MASK 0x7F
#define QUSB_ENDPOINT_NUM(x) (x & QUSB_ENDPOINT_NUM_MASK)
#define QUSB_ENDPOINT_IS_TX(x) ((x & QUSB_ENDPOINT_ADDR_IN_BIT) != 0)
#define QUSB_ENDPOINT_ADDR_OUT(x) (x)
#define QUSB_ENDPOINT_ADDR_IN(x) (QUSB_ENDPOINT_ADDR_IN_BIT | (x))

// USB Endpoint Descriptor bmAttributes bit definitions - Table 9-13
// bits 1..0 : transfer type
#define QUSB_ENDPOINT_ATTR_CONTROL 0x00
#define QUSB_ENDPOINT_ATTR_ISOCHRONOUS 0x01
#define QUSB_ENDPOINT_ATTR_BULK 0x02
#define QUSB_ENDPOINT_ATTR_INTERRUPT 0x03
#define QUSB_ENDPOINT_ATTR_TYPE 0x03
// bits 3..2 : Sync type (only if ISOCHRONOUS)
#define QUSB_ENDPOINT_ATTR_NOSYNC 0x00
#define QUSB_ENDPOINT_ATTR_ASYNC 0x04
#define QUSB_ENDPOINT_ATTR_ADAPTIVE 0x08
#define QUSB_ENDPOINT_ATTR_SYNC 0x0C
#define QUSB_ENDPOINT_ATTR_SYNCTYPE 0x0C
// bits 5..4 : usage type (only if ISOCHRONOUS)
#define QUSB_ENDPOINT_ATTR_DATA 0x00
#define QUSB_ENDPOINT_ATTR_FEEDBACK 0x10
#define QUSB_ENDPOINT_ATTR_IMPLICIT_FEEDBACK_DATA 0x20
#define QUSB_ENDPOINT_ATTR_USAGETYPE 0x30

// Table 9-15 specifies String Descriptor Zero
// Table 9-16 specified UNICODE String Descriptor
typedef struct qusb_string_desc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wData[];
} __attribute__((packed)) qusb_string_desc;

enum _qusb_language_id {
    QUSB_LANGID_ENGLISH_US = 0x409,
};
