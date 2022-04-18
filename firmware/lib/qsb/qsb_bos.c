//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
//

//
// Code implementing the Binary Device Object Store (BOS) requests.
//

#include "qsb_private.h"

#if QSB_BOS == 1

#include <string.h>

static uint32_t build_descriptor(qsb_device* dev, uint8_t* buf);

void qsb_dev_init_bos(qsb_device* device, const qsb_bos_device_capability_desc* const* descs, int num_descs)
{
    device->bos_descs = descs;
    device->num_bos_descs = num_descs;
}

#define APPEND_TO_DESC(data_ptr, data_len)  \
    memcpy(buf_end, data_ptr, data_len);  \
    buf_end += data_len;

uint32_t build_descriptor(qsb_device* dev, uint8_t* buf)
{
    uint8_t* buf_end = buf;

    qsb_bos_desc root_desc = {
        .bLength = sizeof(qsb_bos_desc),
        .bDescriptorType = QSB_DT_BOS,
        .wTotalLength = 0,
        .bNumDeviceCaps = dev->num_bos_descs
    };

    APPEND_TO_DESC(&root_desc, sizeof(root_desc))

    for (int i = 0; i < dev->num_bos_descs; i++) {
        APPEND_TO_DESC(dev->bos_descs[i], dev->bos_descs[i]->bLength)
    }

    uint32_t length = buf_end - buf;

    // use memcpy() as buffer might not be word aligned
    memcpy(buf + 2, &length, sizeof(uint16_t));

    return length;
}

#undef APPEND_TO_DESC

qsb_request_return_code qsb_internal_bos_request_get_desc(
    qsb_device* dev, __attribute__((unused)) qsb_setup_data* req, uint8_t** buf, uint16_t* len)
{
    if (dev->num_bos_descs == 0)
        return QSB_REQ_NOTSUPP;

    *len = imin(*len, build_descriptor(dev, *buf));
    return QSB_REQ_HANDLED;
}

#endif
