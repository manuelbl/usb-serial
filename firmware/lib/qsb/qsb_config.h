//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Work derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Configuration of active peripheral code and functions
//

#pragma once

#define QSB_ARCH_FSDEV 1
#define QSB_ARCH_DWC 2

#define QSB_STR_ENC_LATIN1 1
#define QSB_STR_ENC_UTF8 2

// Configuration flags:
// QSB_ARCH: Defines the architecture of the USB peripheral.
//     Valid values are:
//         QSB_ARCH_FSDEV (1): USB full-speed device interface (the USB peripheral without host, OTG and high-speed support)
//         QSB_ARCH_DWC (2): DesignWare Core (usually called USB OTG FS or USB OTG HS)
//
// QSB_FSDEV_SUBTYPE: Defines the subtype of the USB full-speed device interfaces.
//     Valid values depend on the availablity of the USB_LPMCSR and USB_BCDR register
//         1: has neither USB_LPMCSR nor USB_BCDR register
//         2: has USB_LPMCSR but not USB_BCDR register
//         3: has USB_LPMCSR and USB_BCDR register
//
// QSB_FSDEV_BTABLE_TYPE: Indicates the size of a BTABLE entry (for USB full-speed device interface)
//     Valid values are:
//         2: 16-bit organization, i.e. BTABLE is accessed with half-word access
//         4: 32-bit organization, i.e. BTABLE is accessed with word access, the upper half being empty
//
// QSB_STR_ENC: Selects the encoding of strings provided as input for string descriptors.
//     According to the USB standard, string descriptors are always transmitted with UTF-16 encoding.
//     The QSB library however uses a single byte API. If this flag is not defined, `QSB_STR_UTF8` can be
//     defined alternatively. It selects UTF-8 encoding. If none of the flags is defined, the default is Latin-1.
//     Valid values are:
//         1: Latin-1 (aka ISO-8859-1)
//         2: UTF-8
//
// QSB_BOS_ENABLE: If defined, enables support for USB Binary Device Object Store (BOS)
//     By default, BOS support is not available.
//
//
// QSB_MAX_CONTROL_CALLBACKS: Maximum number of control request callbacks that can be registered.
//     By default, it is 4.
//
// QSB_MAX_SET_CONFIG_CALLBACKS: Maximum number of "Set_Config" request callbackss that can be registered.
//     By default, it is 4.
//
// QSB_WIN_WCID_ENABLE: If defined, enables support for Microsoft WCID descriptors.
//     The WCID descriptors declare that Windows should use the WinUSB drivers. This is good for device
//     with a single interface with number 0. 
//
// QSB_WIN_WCID_VENDOR_CODE: If WCID descriptors is enabled, this macro can be defined to set the
//     vendor code used in the WCID control request. The default value is 0xF0. 

#if !defined(QSB_ARCH)
#if defined(STM32F0)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 3
#elif defined(STM32F1)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 1
#elif defined(STM32F2)
#define QSB_ARCH QSB_ARCH_DWC
#elif defined(STM32F3)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 2
#elif defined(STM32F4)
#define QSB_ARCH QSB_ARCH_DWC
#elif defined(STM32F7)
#define QSB_ARCH QSB_ARCH_DWC
#elif defined(STM32L0)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 3
#elif defined(STM32L1)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 1
#elif defined(STM32L4)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 3
#elif defined(STM32G4)
#define QSB_ARCH QSB_ARCH_FSDEV
#define QSB_FSDEV_SUBTYPE 3
#else
#error "Please define QSB_ARCH"
#endif
#endif

#if QSB_ARCH == QSB_ARCH_FSDEV && !defined(QSB_FSDEV_BTABLE_TYPE)
#if defined(STM32F0)
#define QSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32F1)
#define QSB_FSDEV_BTABLE_TYPE 4
#elif defined(STM32F3)
#define QSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32L0)
#define QSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32L1)
#define QSB_FSDEV_BTABLE_TYPE 4
#elif defined(STM32L4)
#define QSB_FSDEV_BTABLE_TYPE 2
#elif defined(STM32G4)
#define QSB_FSDEV_BTABLE_TYPE 2
#else
#error "Invalid configuration value for QSB_ARCH"
#endif
#endif

#ifndef QSB_STR_ENC
#ifdef QSB_STR_UTF8
#define QSB_STR_ENC QSB_STR_ENC_UTF8
#else
#define QSB_STR_ENC QSB_STR_ENC_LATIN1
#endif
#endif

#ifdef QSB_BOS_ENABLE
#define QSB_BOS 1
#else
#define QSB_BOS 0
#endif

#ifndef QSB_MAX_CONTROL_CALLBACKS
#define QSB_MAX_CONTROL_CALLBACKS 4
#endif

#ifndef QSB_MAX_SET_CONFIG_CALLBACKS
#define QSB_MAX_SET_CONFIG_CALLBACKS 4
#endif

#ifdef QSB_WIN_WCID_ENABLE
#define QSB_WIN_WCID 1
#else
#define QSB_WIN_WCID 0
#endif

#ifndef QSB_WIN_WCID_VENDOR_CODE
#define QSB_WIN_WCID_VENDOR_CODE 0xf0
#endif
