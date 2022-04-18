//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
//

//
// Microsoft Windows specific extensions
//

/**
 * @defgroup qsb_windows Microsoft Windows specific USB extensions
 */


#pragma once

#include "qsb_bos.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup qsb_windows
 *
 * @{
 */


/// Microsoft WCID string index
static const uint8_t QSB_WIN_MSFT_WCID_STR_IDX = 0xee;

/// Microsoft compatible ID feature descriptor request index (wIndex)
static const uint16_t QSB_WIN_COMP_ID_REQ_INDEX = 0x0004;


#if QSB_BOS == 1

/// Microsoft OS 2.0 request `wIndex` value to retrieve MS OS 2.0 vendor-specific descriptor
static const uint8_t QSB_MSOS20_CTRL_INDEX_DESC = 0x07;
/// Microsoft OS 2.0 request `wIndex` value to set alternate enumeration
static const uint8_t QSB_MSOS20_CTRL_INDEX_SET_ALT_ENUM = 0x08;

/// UUID for Microsoft OS 2.0 platform capability: {d8dd60df-4589-4cc7-9cd2-659d9e648a9f}
#define QSB_PLATFORM_CAPABILITY_MICROSOFT_OS20_UUID {0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C, 0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F}

/// Microsoft OS 2.0 descriptor types
typedef enum {
    /// Microsoft OS 2.0 descriptor type for set header
    QSB_MSOS20_DT_SET_HEADER_DESCRIPTOR	= 0x00,
    /// Microsoft OS 2.0 descriptor type for configuration subset header
    QSB_MSOS20_DT_SUBSET_HEADER_CONFIGURATION = 0x01,
    /// Microsoft OS 2.0 descriptor type for function subset header
    QSB_MSOS20_DT_SUBSET_HEADER_FUNCTION = 0x02,
    /// Microsoft OS 2.0 feature descriptor type for compatible ID descriptor
    QSB_MSOS20_DT_FEATURE_COMPATBLE_ID = 0x03,
    /// Microsoft OS 2.0 feature descriptor type for registry propery descriptor
    QSB_MSOS20_DT_FEATURE_REG_PROPERTY = 0x04,
    /// Microsoft OS 2.0 feature descriptor type for minimum USB resume time descriptor
    QSB_MSOS20_DT_FEATURE_MIN_RESUME_TIME = 0x05,
    /// Microsoft OS 2.0 feature descriptor type for model ID descriptor
    QSB_MSOS20_DT_FEATURE_MODEL_ID = 0x06,
    /// Microsoft OS 2.0 feature descriptor type for CCGP device descriptor
    QSB_MSOS20_DT_FEATURE_CCGP_DEVICE = 0x07,
    /// Microsoft OS 2.0 feature descriptor type for vendor revision descriptor
    QSB_MSOS20_DT_FEATURE_VENDOR_REVISION = 0x08,
} qsb_msos20_desc_type_e;

/// Microsoft OS 2.0 property types
typedef enum {
    /// A NULL-terminated Unicode String (REG_SZ)
    QSB_MSOS20_PROP_DATA_TYPE_STRING = 1,
    /// A NULL-terminated Unicode String that includes environment variables (REG_EXPAND_SZ)
    QSB_MSOS20_PROP_DATA_TYPE_STRING_EXPAND = 2,
    /// Free-form binary (REG_BINARY)
    QSB_MSOS20_PROP_DATA_TYPE_BINARY = 3,
    /// A little-endian 32-bit integer (REG_DWORD_LITTLE_ENDIAN)
    QSB_MSOS20_PROP_DATA_TYPE_INT32LE = 4,
    /// A big-endian 32-bit integer (REG_DWORD_BIG_ENDIAN)
    QSB_MSOS20_PROP_DATA_TYPE_INT32BE = 5,
    /// A NULL-terminated Unicode string that contains a symbolic link (REG_LINK)
    QSB_MSOS20_PROP_DATA_TYPE_STRING_LINK = 6,
    /// Multiple NULL-terminated Unicode strings (REG_MULTI_SZ)
    QSB_MSOS20_PROP_DATA_TYPE_STRING_MULTI = 7,
} qsb_msos20_prop_data_type_e;

/// Microsoft OS 2.0 descriptor Windows version
typedef enum {
    /// Windows version 8.1
    QSB_MSOS20_WIN_VER_8_1 = 0x06030000,
    /// Windows version 10
    QSB_MSOS20_WIN_VER_10  = 0x0a000000,
} qsb_msos20_win_ver_e;

/// USB BOS device capability platform descriptor for Microsoft OS 2.0
typedef struct qsb_msos20_platform_desc {
    /// Size of this descriptor
    uint8_t bLength;
    /// Type of this descriptor (use \ref QSB_DT_DEVICE_CAPABILITY)
    uint8_t bDescriptorType;
    /// Device capability type (use \ref QSB_DEV_CAPA_PLATFORM)
    uint8_t bDevCapabilityType;
    /// Reserved. Set to 0.
    uint8_t bReserved;
    /// A 128-bit number / UUID (use \ref QSB_PLATFORM_CAPABILITY_MICROSOFT_OS20_UUID)
    uint8_t platformCapabilityUUID[16];
    /// Minimum Windows version (see \ref qsb_msos20_win_ver_e)
    uint32_t dwWindowsVersion;
    /// The length, in bytes, of the MS OS 2.0 descriptor set
    uint16_t wMSOSDescriptorSetTotalLength;
    /// Vendor defined code to use to retrieve this version of the MS OS 2.0 descriptor and also to set alternate enumeration behavior on the device
    uint8_t bMS_VendorCode;
    /// A non-zero value to send to the device to indicate that the device may return non-default USB descriptors for enumeration. If the device does not support alternate enumeration, this value shall be 0.
    uint8_t bAltEnumCode;
} __attribute__((packed)) qsb_msos20_platform_desc;

/// Microsoft OS 2.0 descriptor set header
typedef struct qsb_msos20_desc_set_header
{
    /// The length, in bytes, of this header
    uint16_t wLength;
    /// The type of this descriptor (use \ref QSB_MSOS20_DT_SET_HEADER_DESCRIPTOR)
    uint16_t wDescriptorType;
    /// Windows version (see \ref qsb_msos20_win_ver_e)
    uint32_t dwWindowsVersion;
    /// The size of entire MS OS 2.0 descriptor set. The value shall match the value in the descriptor set information structure.
    uint16_t wTotalLength;
} __attribute__((packed)) qsb_msos20_desc_set_header;

/// Microsoft OS 2.0 compatible ID descriptor
typedef struct qsb_msos20_desc_compatible_id
{
    /// The length, in bytes, of this header
    uint16_t wLength;
    /// The type of this descriptor (use \ref QSB_MSOS20_DT_FEATURE_COMPATBLE_ID)
    uint16_t wDescriptorType;
    /// Compatible ID string
    char compatibleID[8];
    /// Sub-compatible ID string
    char subCompatibleID[8];
} __attribute__((packed)) qsb_msos20_desc_compatible_id;

#endif

/**
 * @}
 */


#ifdef __cplusplus
}
#endif 
