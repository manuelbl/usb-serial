//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// BTABLE access for STM USB full-speed device, type 4.
// Type 4 has a packet memory area (PMA) that is accessed with 32-bit words
// by the CPU and 16-bit half words by the USB peripheral. The upper half of
// the 32-bit is always 0. Thus relative PMA addresses differ between CPU and USB
// peripheral by a factor of 2.
//

#include "qusb_config.h"

#if QUSB_ARCH == QUSB_ARCH_FSDEV && QUSB_FSDEV_BTABLE_TYPE == 4

#include "qusb_drv_fsdev_btable.h"
#include <libopencm3/stm32/memorymap.h>

// --- USB BTABLE Registers ------------------------------------------------

// This library always puts the BTABLE at pos 0 within PMA
// and uses the index equal to the endpoint address (without direction bit).
// The below macros are simplified based on this assumption.

typedef struct {
    volatile uint32_t addr;
    volatile uint32_t count;
} __attribute__((packed)) buf_desc;

static inline buf_desc* get_buf_desc(uint8_t ep, uint8_t offset)
{
    return (buf_desc*)(USB_PMA_BASE + (ep << 4) + (offset << 3));
}

static inline volatile void* get_pma_addr(buf_desc* desc)
{
    return (volatile void*)(USB_PMA_BASE + desc->addr * 2);
}

void qusb_fsdev_setup_buf_rx(uint8_t ep, qusb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top)
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

void qusb_fsdev_setup_buf_tx(uint8_t ep, qusb_buf_desc_offset offset, uint32_t size, uint16_t* pm_top)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    desc->addr = *pm_top;
    desc->count = 0;
    *pm_top += size;
}

uint32_t qusb_fsdev_get_len(uint8_t ep, qusb_buf_desc_offset offset)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    return desc->count & 0x3ff;
}

void qusb_fsdev_copy_to_pma(uint8_t ep, qusb_buf_desc_offset offset, const uint8_t* buf, uint32_t len)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    desc->count = len;

    volatile uint32_t* tgt = get_pma_addr(desc);
    const uint16_t* src = (const uint16_t*)buf;

    for (; len >= 2; len -= 2)
        *tgt++ = *src++;

    if (len > 0)
        *tgt = *(uint8_t*)src;
}

uint32_t qusb_fsdev_copy_from_pma(uint8_t* buf, uint32_t len, uint8_t ep, qusb_buf_desc_offset offset)
{
    buf_desc* desc = get_buf_desc(ep, offset);
    len = min_u32(len, desc->count & 0x3ff);

    const volatile uint32_t* src = get_pma_addr(desc);
    uint16_t* tgt = (uint16_t*)buf;

    for (unsigned i = 0; i < len >> 1; i++)
        *tgt++ = *src++;

    if ((len & 1) != 0)
        *(uint8_t*)tgt = *src;

    return len;
}

#endif
