//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Work derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Configuration of active peripheral code and functions
//

#pragma once

#define QUSB_ARCH_FSDEV 1
#define QUSB_ARCH_DWC 2

// Configuration flags:
// QUSB_ARCH: Defines the architecture of the USB peripheral.
//     Valid values are:
//         QUSB_ARCH_FSDEV (1): USB full-speed device interface (the USB peripheral without host, OTG and high-speed support)
//         QUSB_ARCH_DWC (2): DesignWare Core (usually called USB OTG FS or USB OTG HS)
//
// QUSB_FSDEV_SUBTYPE: Defines the subtype of the USB full-speed device interfaces.
//     Valid values depend on the availablity of the USB_LPMCSR and USB_BCDR register
//         1: has neither USB_LPMCSR nor USB_BCDR register
//         2: has USB_LPMCSR but not USB_BCDR register
//         3: has USB_LPMCSR and USB_BCDR register
//
// QUSB_FSDEV_BTABLE_TYPE: Indicates the size of a BTABLE entry (for USB full-speed device interface)
//     Valid values are:
//         2: 16-bit organization, i.e. BTABLE is accessed with half-word access
//         4: 32-bit organization, i.e. BTABLE is accessed with word access, the upper half being empty

#if !defined(QUSB_ARCH)
#if defined(STM32F0)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 3
#elif defined(STM32F1)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 1
#elif defined(STM32F2)
#define QUSB_ARCH QUSB_ARCH_DWC
#elif defined(STM32F3)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 2
#elif defined(STM32F4)
#define QUSB_ARCH QUSB_ARCH_DWC
#elif defined(STM32F7)
#define QUSB_ARCH QUSB_ARCH_DWC
#elif defined(STM32L0)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 3
#elif defined(STM32L1)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 1
#elif defined(STM32L4)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 3
#elif defined(STM32G4)
#define QUSB_ARCH QUSB_ARCH_FSDEV
#define QUSB_FSDEV_SUBTYPE 3
#else
#error "Please define QUSB_ARCH"
#endif
#endif

#if QUSB_ARCH == QUSB_ARCH_FSDEV && !defined(QUSB_FSDEV_BTABLE_TYPE)
#if defined(STM32F0)
#define QUSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32F1)
#define QUSB_FSDEV_BTABLE_TYPE 4
#elif defined(STM32F3)
#define QUSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32L0)
#define QUSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32L1)
#define QUSB_FSDEV_BTABLE_TYPE 4
#elif defined(STM32L4)
#define QUSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32G4)
#define QUSB_FSDEV_BTABLE_TYPE 2
#else
#error "Invalid configuration value for QUSB_ARCH"
#endif
#endif
