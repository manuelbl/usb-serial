//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// BTABLE access for STM USB full-speed device, type 2.
// Type 2 has a packet memory area (PMA) that is accessed with 16-bit half
// words by both the CPU and the USB peripheral. Thus relative PMA addresses are
// the same for both.
// Acessing two half words with a single 32-bit operation does not work and leads
// to unexpected results. It must always be access with 16-bit half word operations.
//

#include "qsb_config.h"

#if QSB_ARCH == QSB_ARCH_FSDEV && QSB_FSDEV_BTABLE_TYPE == 2

#include "qsb_drv_fsdev_btable.h"
#include <libopencm3/stm32/memorymap.h>

// --- USB BTABLE Registers ------------------------------------------------

// This library always puts the BTABLE at pos 0 within PMA
// and uses the index equal to the endpoint address (without direction bit).
// The below macros are simplified based on this assumption.

typedef struct {
    volatile uint16_t addr;
    volatile uint16_t count;
} __attribute__((packed)) buf_desc;

static inline buf_desc* get_buf_desc(uint8_t ep, uint8_t offset)
{
    return (buf_desc*)(USB_PMA_BASE + (ep << 3) + (offset << 2));
}

static inline volatile void* get_pma_addr(buf_desc* desc)
{
    return (volatile void*)(USB_PMA_BASE + desc->addr);
}

void qsb_fsdev_setup_buf_rx(uint8_t ep, qsb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top)
{
    uint32_t eff_size;
    if (size > 62) {
        // Bigger than 62: use BL_SIZE = 1 / block_size = 32

        // Round up, div by 32 and sub 1 == (size + 31)/32 - 1 == (size-1)/32)
        size = ((size - 1) >> 5) & 0x1F;
        eff_size = (size + 1) << 5;
        // Set BL_SIZE bit
        size |= 1 << 5;
    } else {
        // Smaller or equal to 62: use BL_SIZE = 0 / block_size = 2

        // round up and div by 2
        size = (size + 1) >> 1;
        eff_size = size << 1;
    }
    buf_desc* desc = get_buf_desc(ep, offset);
    // write to the BL_SIZE and NUM_BLOCK fields
    desc->count = size << 10;
    desc->addr = *pm_top;
    *pm_top += eff_size;
}

void qsb_fsdev_setup_buf_tx(uint8_t ep, qsb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    desc->addr = *pm_top;
    desc->count = 0;
    *pm_top += size;
}

uint32_t qsb_fsdev_get_len(uint8_t ep, qsb_buf_desc_offset offset)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    return desc->count & 0x3ff;
}

void qsb_fsdev_copy_to_pma(uint8_t ep, qsb_buf_desc_offset offset, const uint8_t* buf, uint32_t len)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    desc->count = len;

    volatile uint16_t* tgt = get_pma_addr(desc);

    for (unsigned i = 0; i + 1 < len; i += 2)
        *tgt++ = (buf[i + 1] << 8) | buf[i];

    if ((len & 1) != 0)
        *tgt = buf[len - 1];
}

uint32_t qsb_fsdev_copy_from_pma(uint8_t* buf, uint32_t len, uint8_t ep, qsb_buf_desc_offset offset)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    len = imin(len, desc->count & 0x3ff);
    const volatile uint16_t* src = get_pma_addr(desc);

    if (((uintptr_t)buf) & 0x01) {
        // target buffer is not half word aligned -> copy byte by byte
        for (unsigned i = 0; i < len >> 1; i++) {
            uint16_t hw = *src++;
            *buf++ = hw;
            *buf++ = hw >> 8;
        }
    } else {
        // target buffer is half word aligned -> copy half word by half word
        uint16_t* tgt = (uint16_t*)buf;
        for (unsigned i = 0; i < len >> 1; i++)
            *tgt++ = *src++;
        buf = (uint8_t*)tgt;
    }

    if ((len & 1) != 0)
        *buf = *src;

    return len;
}

#endif
