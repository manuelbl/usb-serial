//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Functions for generating serial number.
//

#include "qsb_device.h"
#include <string.h>


#ifndef QSB_UID_BASE

#if defined(STM32F0)

#define QSB_UID_BASE 0x1FFFF7AC

#elif defined(STM32F1)

#define QSB_UID_BASE 0x1FFFF7E8

#elif defined(STM32F2)

#define QSB_UID_BASE 0x1FFF7A10

#elif defined(STM32F3)

#define QSB_UID_BASE 0x1FFFF7AC

#elif defined(STM32F4)

#define QSB_UID_BASE 0x1FFF7A10

#elif defined(STM32F7)

#error "The STM32F7 family uses various addresses for the Unique device ID register. Please look it up in the reference manual and define QSB_UID_BASE"

#elif defined(STM32G0)

// The STM32G0 familiy does not provide a unique device ID
#define QSB_UID_BASE 0x0

#elif defined(STM32G4)

#define QSB_UID_BASE 0x1FFF7590

#elif defined(STM32H7)

#define QSB_UID_BASE 0x1FF1E800

#elif defined(STM32L0)

#define QSB_UID_BASE 0x1FF80050

#elif defined(STM32L1)

#define QSB_UID_BASE 0x1FF80050

#elif defined(STM32L4)

#define QSB_UID_BASE 0x1FFF7590

#elif

#error "Unknown processor family; have you defined STM32F1 or a similar macro?"

#endif

#endif


// Sample for UID:
// 42 00 4B 00 18 51 37 34 35 37 38 32
//
// Structure (see RM0351):
// Wafer x-position (2 byte, little endian)
// Wafer y-position (2 byte, little endian)
// Wafer number (1 byte)
// Lot number (7 bytes, ASCII)

static const char* hex_chars = "0123456789ABCDEF";

char qsb_serial_num[9];

const char* qsb_serial_num_init(void) {
    // get unique device ID
    const uint8_t* uid = (const uint8_t*)QSB_UID_BASE;

    // hash unique device ID (Jenkins OAAT)
    uint32_t hash = 0;
    for (int i = 0; i < 12; i++) {
        hash += uid[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    // convert to ASCII string (hex)
    for (int i = 0; i < 8; i++) {
        qsb_serial_num[i] = hex_chars[hash & 0x0f];
        hash >>= 4;
    }
    qsb_serial_num[8] = 0;

    return qsb_serial_num;
}
