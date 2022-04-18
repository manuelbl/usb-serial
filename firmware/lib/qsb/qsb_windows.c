//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
//

//
// Microsoft Windows specific extensions
//

#include "qsb_windows.h"
#include "qsb_private.h"

//
// Microsoft WCID
//
// See https://github.com/pbatard/libwdi/wiki/WCID-Devices
//

#if QSB_WIN_WCID == 1

// Microsoft WCID string descriptor (string index 0xee)
static const uint8_t msft_sig_desc[] = {
    0x12,                           // length = 18 bytes
    QSB_DT_STRING,                  // descriptor type string
    'M', 0, 'S', 0, 'F', 0, 'T', 0, // 'M', 'S', 'F', 'T'
    '1', 0, '0', 0, '0', 0,         // '1', '0', '0'
    QSB_WIN_WCID_VENDOR_CODE,       // vendor code
    0                               // padding
};

// Microsoft WCID feature descriptor (index 0x0004)
static const uint8_t wcid_feature_desc[] = {
    0x28, 0x00, 0x00, 0x00,                         // length = 40 bytes
    0x00, 0x01,                                     // version 1.0 (in BCD)
    0x04, 0x00,                                     // compatibility descriptor index 0x0004
    0x01,                                           // number of sections
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       // reserved (7 bytes)
    0x00,                                           // interface number 0
    0x01,                                           // reserved
    0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00, // Compatible ID "WINUSB\0\0"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Subcompatible ID (unused)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00              // reserved 6 bytes
};

qsb_request_return_code qsb_internal_win_get_msft_string_desc(uint8_t** buf, uint16_t* len)
{
    // Already checked by caller:
    // - request type = device-to-host / standard / device
    // - request == QSB_REQ_GET_DESCRIPTOR (bRequest)
    // - descriptor type == QSB_DT_STRING (high byte of wValue)
    // - descriptor index == QSB_WIN_MSFT_WCID_STR_IDX (low byte of wValue)
    *buf = (uint8_t*) msft_sig_desc;
    *len = imin(*len, msft_sig_desc[0]);
    return QSB_REQ_HANDLED;
}

qsb_request_return_code qsb_internal_win_wcid_vendor_request(qsb_setup_data *req, uint8_t **buf, uint16_t *len)
{
    // 0x0004: Microsoft WCID index for feature descriptor
    if ((req->bmRequestType & QSB_REQ_TYPE_TYPE_MASK) == QSB_REQ_TYPE_VENDOR
        && req->bRequest == QSB_WIN_WCID_VENDOR_CODE
        && req->wIndex == QSB_WIN_COMP_ID_REQ_INDEX)
    {
        *buf = (uint8_t*) &wcid_feature_desc;
        *len = imin(*len, (uint16_t)wcid_feature_desc[0]);
        return QSB_REQ_HANDLED;
    }

    return QSB_REQ_NEXT_HANDLER;
}

#endif
