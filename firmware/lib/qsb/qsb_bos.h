//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
//

//
// Declarations for Binary Device Object Store (BOS) and WebUSB
// Based on:
// - Universal Serial Bus 3.2 Specification
//   (Revision 1.0, September 22, 2017)
// - WebUSB API
//   (Draft Community Group Report, 25 February 2022)
// - Microsoft OS 2.0 Descriptors Specification
//   https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-os-2-0-descriptors-specification
//

/**
 * @defgroup qsb_bos USB binary device object store (BOS)
 */


#pragma once

#include "qsb_config.h"
#include <libopencm3/cm3/common.h>

#if QSB_BOS == 1

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @addtogroup qsb_bos
 *
 * @{
 */

/// USB BOS device capability types
typedef enum {
    /// USB BOS device capability type for Wireless USB-specific device level capabilities
    QSB_DEV_CAPA_WIRELESS_USB = 0x01,
    /// USB BOS device capability type for USB 2.0 extension descriptor
    QSB_DEV_CAPA_USB_2_0_EXTENSION = 0x02,
    /// USB BOS device capability type for SuperSpeed USB specific device level capabilities
    QSB_DEV_CAPA_SUPERSPEED_USB = 0x03,
    /// USB BOS device capability type for instance unique ID used to identify the instance across all operating modes
    QSB_DEV_CAPA_CONTAINER_ID = 0x04,
    /// USB BOS device capability type for device capability specific to a particular platform/operating system
    QSB_DEV_CAPA_PLATFORM = 0x05,
    /// USB BOS device capability type for various PD capabilities of this device
    QSB_DEV_CAPA_POWER_DELIVERY_CAPABILITY = 0x06,
    /// USB BOS device capability type for information on each battery supported by the device
    QSB_DEV_CAPA_BATTERY_INFO_CAPABILITY = 0x07,
    /// USB BOS device capability type for consumer characteristics of a port on the device
    QSB_DEV_CAPA_PD_CONSUMER_PORT_CAPABILITY = 0x08,
    /// USB BOS device capability type for provider characteristics of a port on the device
    QSB_DEV_CAPA_PD_PROVIDER_PORT_CAPABILITY = 0x09,
    /// USB BOS device capability type for SuperSpeed Plus USB specific device level capabilities
    QSB_DEV_CAPA_SUPERSPEED_PLUS = 0x0a,
    /// USB BOS device capability type for precision time measurement (PTM) capability descriptor
    QSB_DEV_CAPA_PRECISION_TIME_MEASUREMENT = 0x0b,
    /// USB BOS device capability type for wireless USB 1.1-specific device level capabilities
    QSB_DEV_CAPA_WIRELESS_USB_EXT = 0x0c,
    /// USB BOS device capability type for billboard capability
    QSB_DEV_CAPA_BILLBOARD = 0x0d,
    /// USB BOS device capability type for authentication capability descriptor
    QSB_DEV_CAPA_AUTHENTICATION = 0x0e,
    /// USB BOS device capability type billboard ex capability
    QSB_DEV_CAPA_BILLBOARD_EX = 0x0f,
    /// USB BOS device capability type for summarizing configuration information for a function implemented by the device
    QSB_DEV_CAPA_CONFIGURATION_SUMMARY = 0x10,
} qsb_dev_capa_type_e;

/// WebUSB device request `wIndex` value for "GET_URL"
static const uint8_t QSB_REQ_WEBUSB_GET_URL = 2;

/// WebUSB URL descriptor type code
static const uint8_t QSB_DT_WEBUSB_URL = 3;

/// WebUSB URL prefixes
typedef enum {
    /// WebUSB URL prefix "http://"
    QSB_WEBUSB_URL_SCHEME_HTTP = 0,
    /// WebUSB URL prefix "https://"
    QSB_WEBUSB_URL_SCHEME_HTTPS = 1,
    /// WebUSB URL prefix "" (none)
    QSB_WEBUSB_URL_SCHEME_NONE = 255,
} qsb_webusb_url_scheme_e;

/// UUID for WebUSD platform capability: {3408b638-09a9-47a0-8bfd-a0768815b665}
#define QSB_PLATFORM_CAPABILITY_WEBUSB_UUID {0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47, 0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65}

/// USB BOS descriptor
typedef struct qsb_bos_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_BOS)
    uint8_t bDescriptorType;
    /// Length of this descriptor and all of its sub descriptors
    uint16_t wTotalLength;
    /// The number of separate device capability descriptors in the BOS
    uint8_t bNumDeviceCaps;
} __attribute__((packed)) qsb_bos_desc;

/// USB BOS device capability descriptor (generic)
typedef struct qsb_bos_device_capability_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_DEVICE_CAPABILITY)
    uint8_t bDescriptorType;
    /// Device capability type (see \ref qsb_dev_capa_type_e)
    uint8_t bDevCapabilityType;
    /// Capability-specific data
    uint8_t data[];
} __attribute__((packed)) qsb_bos_device_capability_desc;

/// USB BOS device capability platform descriptor
typedef struct qsb_bos_platform_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_DEVICE_CAPABILITY)
    uint8_t bDescriptorType;
    /// Device capability type (use \ref QSB_DEV_CAPA_PLATFORM)
    uint8_t bDevCapabilityType;
    /// Reserved. Set to 0.
    uint8_t bReserved;
    /// A 128-bit number (UUID) that uniquely identifies a platform specific capability of the device
    uint8_t platformCapabilityUUID[16];
    /// Platform-specific capability data
    uint8_t capabilityData[];
} __attribute__((packed)) qsb_bos_platform_desc;

/// USB BOS device capability platform descriptor for WebUSB
typedef struct qsb_webusb_platform_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_DEVICE_CAPABILITY)
    uint8_t bDescriptorType;
    /// Device capability type (use \ref QSB_DEV_CAPA_PLATFORM)
    uint8_t bDevCapabilityType;
    /// Reserved. Set to 0.
    uint8_t bReserved;
    /// A 128-bit number / UUID (use \ref QSB_PLATFORM_CAPABILITY_WEBUSB_UUID)
    uint8_t platformCapabilityUUID[16];
    /// Protocol version supported. Must be set to 0x0100.
    uint16_t bcdVersion;
    /// `bRequest` value used for issuing WebUSB requests.
    uint8_t bVendorCode;
    /// URL descriptor index of the device’s landing page.
    uint8_t iLandingPage;
} __attribute__((packed)) qsb_webusb_platform_desc;

/// WebUSB URL descriptor
typedef struct qsb_webusb_url_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_WEBUSB_URL)
    uint8_t bDescriptorType;
    /// URL scheme prefix (see \ref qsb_webusb_url_scheme_e)
    uint8_t bScheme;
    /// UTF-8 encoded URL (excluding the scheme prefix)
    char url[];
} __attribute__((packed)) qsb_webusb_url_desc;


/**
 * @}
 */

#ifdef __cplusplus
}
#endif 

#endif
