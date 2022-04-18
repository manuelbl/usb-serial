//
// QSB USB Device Library for libopencm3
//
// Copyright (c) 2021 Manuel Bleichenbacher
// Copyright (c) 2009 Piotr Esden-Tempski <piotr@esden.net>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Register definitions for STM's USB full-speed device interface.
//

#pragma once

#include <libopencm3/stm32/memorymap.h>

// --- USB general registers -----------------------------------------------

// USB control register
#define USB_CNTR   MMIO32(USB_DEV_FS_BASE + 0x40)
// USB interrupt status register
#define USB_ISTR   MMIO32(USB_DEV_FS_BASE + 0x44)
// USB frame number register
#define USB_FNR    MMIO32(USB_DEV_FS_BASE + 0x48)
// USB device address register
#define USB_DADDR  MMIO32(USB_DEV_FS_BASE + 0x4C)
// USB buffer table address register
#define USB_BTABLE MMIO32(USB_DEV_FS_BASE + 0x50)

// USB endpoint registers
#define USB_EP(EP) MMIO32(USB_DEV_FS_BASE + 4 * (EP))

// --- USB control register masks / bits -----------------------------------

// Interrupt mask bits, set to 1 to enable interrupt generation
#define USB_CNTR_CTRM     0x8000
#define USB_CNTR_PMAOVRM  0x4000
#define USB_CNTR_ERRM     0x2000
#define USB_CNTR_WKUPM    0x1000
#define USB_CNTR_SUSPM    0x0800
#define USB_CNTR_RESETM   0x0400
#define USB_CNTR_SOFM     0x0200
#define USB_CNTR_ESOFM    0x0100

// Request/Force bits
#define USB_CNTR_RESUME   0x0010 // Resume request
#define USB_CNTR_FSUSP    0x0008 // Force suspend
#define USB_CNTR_LP_MODE  0x0004 // Low-power mode
#define USB_CNTR_PWDN     0x0002 // Power down
#define USB_CNTR_FRES     0x0001 // Force reset

// --- USB interrupt status register masks / bits --------------------------

#define USB_ISTR_CTR      0x8000 // Correct transfer
#define USB_ISTR_PMAOVR   0x4000 // Packet memory area overrun/underrund
#define USB_ISTR_ERR      0x2000 // Error
#define USB_ISTR_WKUP     0x1000 // Wake up
#define USB_ISTR_SUSP     0x0800 // Suspend mode request
#define USB_ISTR_RESET    0x0400 // USB RESET request
#define USB_ISTR_SOF      0x0200 // Start Of Frame
#define USB_ISTR_ESOF     0x0100 // Expected Start Of Frame
#define USB_ISTR_DIR      0x0010 // Direction of transaction
#define USB_ISTR_EP_ID    0x000F // Endpoint identifier

// --- USB Frame Number Register bits --------------------------------------

#define USB_FNR_RXDP (1 << 15)
#define USB_FNR_RXDM (1 << 14)
#define USB_FNR_LCK (1 << 13)

#define USB_FNR_LSOF_SHIFT 11
#define USB_FNR_LSOF (3 << USB_FNR_LSOF_SHIFT)

#define USB_FNR_FN (0x7FF << 0)

// --- USB device address register masks / bits ----------------------------

#define USB_DADDR_EF (1 << 7)
#define USB_DADDR_ADDR    0x007F

// USB_BTABLE Values -------------------------------------------------------

#define USB_BTABLE_BTABLE 0xFFF8

// --- USB endpoint register masks / bits ----------------------------------

// Masks and toggle bits
#define USB_EP_CTR_RX     0x8000 // Correct transfer RX
#define USB_EP_DTOG_RX    0x4000 // Data toggle RX
#define USB_EP_SW_BUF_TX  0x4000 // Buffer under control of software (for double-buffered TX)
#define USB_EP_STAT_RX    0x3000 // Endpoint status for RX

#define USB_EP_SETUP      0x0800 // Setup transaction completed
#define USB_EP_TYPE       0x0600 // Endpoint type
#define USB_EP_KIND       0x0100 // Endpoint kind

#define USB_EP_CTR_TX     0x0080 // Correct transfer TX
#define USB_EP_DTOG_TX    0x0040 // Data toggle TX
#define USB_EP_SW_BUF_RX  0x0040 // Buffer under control of software (for double-buffered RX)
#define USB_EP_STAT_TX    0x0030 // Endpoint status for TX

#define USB_EP_ADDR       0x000F // Endpoint address (called "EA" in reference manual)

#define USB_EP_RW_BITS_MSK (USB_EP_TYPE | USB_EP_KIND | USB_EP_ADDR)
#define USB_EP_W0_BITS_MSK (USB_EP_CTR_RX | USB_EP_CTR_TX)

// Endpoint status bits for USB_EP_STAT_RX bit field
#define USB_EP_STAT_RX_DISABLED 0x0000
#define USB_EP_STAT_RX_STALL    0x1000
#define USB_EP_STAT_RX_NAK      0x2000
#define USB_EP_STAT_RX_VALID    0x3000

// Endpoint status bits for USB_EP_STAT_TX bit field
#define USB_EP_STAT_TX_DISABLED 0x0000
#define USB_EP_STAT_TX_STALL    0x0010
#define USB_EP_STAT_TX_NAK      0x0020
#define USB_EP_STAT_TX_VALID    0x0030

// Endpoint type bits for USB_EP_TYPE bit field
#define USB_EP_TYPE_BULK        0x0000
#define USB_EP_TYPE_CONTROL     0x0200
#define USB_EP_TYPE_ISO         0x0400
#define USB_EP_TYPE_INTERRUPT   0x0600

// Endpoint kind bits for USB_EP_KIND
#define USB_EP_KIND_DBL_BUF     0x0100 // Double buffering

#if QSB_FSDEV_SUBTYPE >= 2

// --- Link power management register definitions ----------------------------------------------

#define USB_LPMCSR MMIO32(USB_DEV_FS_BASE + 0x54)

// --- USB control register masks / bits -----------------------------------

#define USB_CNTR_L1REQM    (1 << 7)
#define USB_CNTR_L1RESUME  (1 << 5)

// --- USB interrupt status register masks / bits --------------------------

#define USB_ISTR_L1REQ     (1 << 7)

// --- LPM control and status register USB_LPMCSR values --------------------

#define USB_LPMCSR_BESL_SHIFT 4
#define USB_LPMCSR_BESL    (15 << USB_LPMCSR_BESL_SHIFT)

#define USB_LPMCSR_REMWAKE (1 << 3)
#define USB_LPMCSR_LPMACK  (1 << 1)
#define USB_LPMCSR_LPMEN   (1 << 0)

#endif

#if QSB_FSDEV_SUBTYPE >= 3

// --- Battery charging register definitions ----------------------------------------------------------

#define USB_BCDR MMIO32(USB_DEV_FS_BASE + 0x58)

// --- Battery charging detector values ----------------------------------------------------------

#define USB_BCDR_DPPU      (1 << 15)
#define USB_BCDR_PS2DET    (1 << 7)
#define USB_BCDR_SDET      (1 << 6)
#define USB_BCDR_PDET      (1 << 5)
#define USB_BCDR_DCDET     (1 << 4)
#define USB_BCDR_SDEN      (1 << 3)
#define USB_BCDR_PDEN      (1 << 2)
#define USB_BCDR_DCDEN     (1 << 1)
#define USB_BCDR_BCDEN     (1 << 0)

#endif
