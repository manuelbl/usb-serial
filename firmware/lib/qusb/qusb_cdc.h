//
// Qusb USB Device Library for libopencm3
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Copyright (c) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Licensed under LGPL License https://opensource.org/licenses/LGPL-3.0
// Derived from libopencm3 (https://github.com/libopencm3/libopencm3)
//

//
// Declarations for USB Communication Device Class (CDC)
// Based on:
// - Universal Serial Bus Class Definitions for Communications Devices
//   (Revision 1.2, December 6, 2012)
// - Universal Serial Bus Class Subclass Specification for PSTN Devices
//   (Revision 1.2, February 9, 2007)
//

#pragma once

#include <libopencm3/cm3/common.h>


// USB CDC table 2: Communications device class code
#define QUSB_CLASS_CDC                 0x02

// USB CDC table 3: Communications interface class code
#define QUSB_INTERFACE_CDC             0x02

// USB CDC table 4: Class subclass codes
#define QUSB_CDC_SUBCLASS_DLCM         0x01
#define QUSB_CDC_SUBCLASS_ACM          0x02

// USB CDC table 5: Communications interface class control protocol codes
#define QUSB_CDC_PROTOCOL_NONE         0x00
#define QUSB_CDC_PROTOCOL_AT           0x01

// USB CDC table 6: Data interface class code
#define QUSB_CDC_CLASS_DATA            0x0A

// USB CDC table 12: Type values for the bDescriptorType field
#define QUSB_CDC_CS_INTERFACE          0x24
#define QUSB_CDC_CS_ENDPOINT           0x25

// USB CDC table 13: bDescriptor sub type in communications class functional descriptors
#define QUSB_CDC_TYPE_HEADER           0x00
#define QUSB_CDC_TYPE_CALL_MANAGEMENT  0x01
#define QUSB_CDC_TYPE_ACM              0x02

#define QUSB_CDC_TYPE_UNION		0x06

// USB CDC table 15: Class-specific descriptor header format

/// USB CDC header functional descriptor
struct qusb_cdc_header_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// QUSB_CDC_CS_INTERFACE descriptor type
	uint8_t bDescriptorType;
    /// Header functional descriptor subtype (QUSB_CDC_TYPE_xxx)
	uint8_t bDescriptorSubtype;
    /// USB CDC specification release number in binary-coded decimal (0x0110)
	uint16_t bcdCDC;
} __attribute__((packed));

// USB CDC table 16: Union interface functional descriptor

/// USB CDC union functional descriptor
struct qusb_cdc_union_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// QUSB_CDC_CS_INTERFACE descriptor type
	uint8_t bDescriptorType;
    /// Header functional descriptor subtype (QUSB_CDC_TYPE_xxx)
	uint8_t bDescriptorSubtype;
	/// The interface number of the communications or data class interface
	uint8_t bControlInterface;
	/// The interface number of the first subordinate interface in the union
	uint8_t bSubordinateInterface0;
} __attribute__((packed));

/// Notification Structure
struct qusb_cdc_notification {
	uint8_t bmRequestType;
	uint8_t bNotification;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint8_t data[0];
} __attribute__((packed));


// USB PSTN table 4: Abstract control management functional descriptor (field bmCapabilities)

/// Device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature
#define QUSB_ACM_CAP_COMM_FEATURES     1
/// Device supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State
#define QUSB_ACM_CAP_LINE_CODING       2
/// Device supports the request Send_Break
#define QUSB_ACM_CAP_SEND_BREAK        4
/// Device supports the notification Network_Connection
#define QUSB_ACM_CAP_NET_CONN_NOTIF    8

// USB PSTN table 13: Class-specific request codes for PSTN subclasses
#define QUSB_PSTN_REQ_SET_LINE_CODING  0x20
#define QUSB_PSTN_REQ_GET_LINE_CODING  0x21
#define QUSB_PSTN_REQ_SET_CONTROL_LINE_STATE 0x22


// USB PSTN table 3: Call management functional descriptor

/// USB PSTN call management functional descriptor
struct qusb_pstn_call_management_desc {
	/// Size of this functional descriptor, in bytes
	uint8_t bFunctionLength;
    /// QUSB_CDC_CS_INTERFACE descriptor type
	uint8_t bDescriptorType;
	/// Call management functional descriptor subtype (QUSB_CDC_TYPE_CALL_MANAGEMENT)
	uint8_t bDescriptorSubtype;
	/// The capabilities that this configuration supports
	uint8_t bmCapabilities;
	/// Interface nmber of the Data Class interface optionally used for call management
	uint8_t bDataInterface;
} __attribute__((packed));

// USB PSTN table 4: Abstract control management functional descriptor

/// USB PSTN Abstract control management functional descriptor
struct qusb_cdc_acm_desc {
	/// Size of this functional descriptor, in bytes
	uint8_t bFunctionLength;
    /// QUSB_CDC_CS_INTERFACE descriptor type
	uint8_t bDescriptorType;
	/// Call management functional descriptor subtype (QUSB_CDC_TYPE_ACM)
	uint8_t bDescriptorSubtype;
	/// The capabilities that this configuration supports (QUSB_ACM_CAP_xxx)
	uint8_t bmCapabilities;
} __attribute__((packed));

// USB PSTN table 17: Line coding structure

/// USB PSTN line coding structure
struct qusb_pstn_line_coding {
	/// Data terminal rate, in bits per second
	uint32_t dwDTERate;
	/// Stop bits (0: 1 stop bit; 1: 1.5 stop bits; 2: 2 stop bits)
	uint8_t bCharFormat;
	/// Parity (0: none; 1: odd; 2: even; 3: mark; 4: space)
	uint8_t bParityType;
	/// Data bits (5, 6, 7, 8 or 16)
	uint8_t bDataBits;
} __attribute__((packed));

/// USB PSTN char format
enum qusb_pstn_line_coding_char_format {
    QUSB_PSTN_1_STOP_BITS     = 0,
    QUSB_PSTN_1_5_STOP_BITS   = 1,
    QUSB_PSTN_2_STOP_BITS     = 2
};

/// USB PSTN parity type
enum qusb_pstn_line_coding_parity_type {
    QUSB_PSTN_NO_PARITY       = 0,
    QUSB_PSTN_ODD_PARITY      = 1,
    QUSB_PSTN_EVEN_PARITY     = 2,
    QUSB_PSTN_MARK_PARITY     = 3,
    QUSB_PSTN_SPACE_PARITY    = 4
};

// USB PSTN table 30: Subclass specific notifications
#define QUSB_PSTN_NOTIF_SERIAL_STATE   0x20

// USB PSTN table 31: UART state bitmap values

/// State of receiver carrier detection mechanism of device. This signal corresponds to V.24 signal 109 and RS-232 signal DCD
#define QUSB_PSTN_UART_STATE_RX_CARRIER  0x01
/// State of transmission carrier. This signal corresponds to V.24 signal 106 and RS-232 signal DSR
#define QUSB_PSTN_UART_STATE_TX_CARRIER  0x02
/// State of break detection mechanism of the device
#define QUSB_PSTN_UART_STATE_BREAK       0x04
/// State of ring signal detection of the device
#define QUSB_PSTN_UART_STATE_RING_SIGNAL 0x08
/// A framing error has occurred
#define QUSB_PSTN_UART_STATE_FRAMING     0x10
/// A parity error has occurred
#define QUSB_PSTN_UART_STATE_PARITY      0x20
/// Received data has been discarded due to overrun in the device
#define QUSB_PSTN_UART_STATE_OVERRUN     0x40

// USB PSTN ch 6.5.4: Serial state notification structure

/// USB PSTN serial state notification structure
struct qusb_pstn_serial_state_notif {
	/// Request type (QUSB_REQ_TYPE_IN | QUSB_REQ_TYPE_CLASS | QUSB_REQ_TYPE_INTERFACE)
	uint8_t bmRequestType;
	/// Notification code (QUSB_PSTN_NOTIF_SERIAL_STATE)
	uint8_t bNotification;
	/// Value (0)
	uint16_t wValue;
	/// Index (interface number)
	uint16_t wIndex;
	/// Length of data (2)
	uint16_t wLength;
	/// Serial/UART state (QUSB_PSTN_UART_STATE_xxx)
	uint16_t wSerialState;
} __attribute__((packed));
