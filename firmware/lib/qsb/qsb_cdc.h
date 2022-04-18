//
// QSB USB Device Library for libopencm3
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

/**
 * @defgroup qsb_cdc USB CDC ACM (USB serial)
 */


#pragma once

#include <libopencm3/cm3/common.h>

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @addtogroup qsb_cdc
 *
 * @{
 */


// USB CDC table 3: Communications interface class code

/// USB CDC interface class "Communication"
static const uint8_t QSB_CDC_INTF_CLASS_COMM = 0x02;

// USB CDC table 4: Class subclass codes

/// USB CDC interface subclass "DLCM"
static const uint8_t QSB_CDC_INTF_SUBCLASS_DLCM = 0x01;
/// USB CDC interface subclass "ACM"
static const uint8_t QSB_CDC_INTF_SUBCLASS_ACM = 0x02;

// USB CDC table 5: Communications interface class control protocol codes

/// USB CDC interface protocol "none"
static const uint8_t QSB_CDC_INTF_PROTOCOL_NONE = 0x00;
/// USB CDC interface protocol "AT"
static const uint8_t QSB_CDC_INTF_PROTOCOL_AT = 0x01;

// USB CDC table 6: Data interface class code

/// USB CDC interface class "Data"
static const uint8_t QSB_CDC_INTF_CLASS_DATA = 0x0A;

// USB CDC table 12: Type values for the bDescriptorType field

/// USB CDC functional descriptor type "Interface"
static const uint8_t QSB_CDC_FUNC_DT_INTERFACE = 0x24;
/// USB CDC functional descriptor type "Endpoint"
static const uint8_t QSB_CDC_FUNC_DT_ENDPOINT = 0x25;

// USB CDC table 13: bDescriptorsubType in communications class functional descriptors

/// USB CDC functional descriptor subtype "Header"
static const uint8_t QSB_CDC_FUNC_SUBTYPE_HEADER = 0x00;
/// USB CDC functional descriptor subtype "Call Management"
static const uint8_t QSB_CDC_FUNC_SUBTYPE_CALL_MANAGEMENT = 0x01;
/// USB CDC functional descriptor subtype "Abstract Call Management"
static const uint8_t QSB_CDC_FUNC_SUBTYPE_ACM = 0x02;
/// USB CDC functional descriptor subtype "Union"
static const uint8_t QSB_CDC_FUNC_SUBTYPE_UNION = 0x06;

// USB CDC table 15: Class-specific descriptor header format

/// USB CDC header functional descriptor
typedef struct qsb_cdc_header_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// Type of this descriptor (use \ref QSB_CDC_FUNC_DT_INTERFACE)
	uint8_t bDescriptorType;
    /// Subtype of this descriptor (use \ref QSB_CDC_FUNC_SUBTYPE_HEADER)
	uint8_t bDescriptorSubtype;
    /// USB CDC specification release number in binary-coded decimal (0x0110)
	uint16_t bcdCDC;
} __attribute__((packed)) qsb_cdc_header_desc;

// USB CDC table 16: Union interface functional descriptor

/// USB CDC union functional descriptor
typedef struct qsb_cdc_union_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// Type of this descriptor (use \ref QSB_CDC_FUNC_DT_INTERFACE)
	uint8_t bDescriptorType;
    /// Subtype of this descriptor (use \ref QSB_CDC_FUNC_SUBTYPE_UNION)
	uint8_t bDescriptorSubtype;
	/// The interface number of the communications or data class interface
	uint8_t bControlInterface;
	/// The interface number of the first subordinate interface in the union
	uint8_t bSubordinateInterface0;
} __attribute__((packed)) qsb_cdc_union_desc;

/// Notification Structure
typedef struct qsb_cdc_notification {
    /// Characteristics of notification (see \ref qsb_req_type_e)
    uint8_t bmRequestType;
	/// Notification code
	uint8_t bNotification;
    /// Word-sized field that varies according to the notification
    uint16_t wValue;
    /// Word-sized field that varies according to notification
    uint16_t wIndex;
    /// Length of `data` field (in bytes)
	uint16_t wLength;
	/// Variable length data according to notification
	uint8_t data[];
} __attribute__((packed)) qsb_cdc_notification;


// USB PSTN table 4: Abstract control management functional descriptor (field bmCapabilities)

/// ACM functional descriptor capability: Device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature
static const uint8_t QSB_ACM_CAP_COMM_FEATURES = 1;
/// ACM functional descriptor capability: Device supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State
static const uint8_t QSB_ACM_CAP_LINE_CODING = 2;
/// ACM functional descriptor capability: Device supports the request Send_Break
static const uint8_t QSB_ACM_CAP_SEND_BREAK = 4;
/// ACM functional descriptor capability: Device supports the notification Network_Connection
static const uint8_t QSB_ACM_CAP_NET_CONN_NOTIF = 8;

// USB PSTN table 13: Class-specific request codes for PSTN subclasses

/// PSTN specific request code for setting line coding
static const uint8_t QSB_PSTN_REQ_SET_LINE_CODING = 0x20;
/// PSTN specific request code for getting line coding
static const uint8_t QSB_PSTN_REQ_GET_LINE_CODING = 0x21;
/// PSTN specific request code for setting the control line state
static const uint8_t QSB_PSTN_REQ_SET_CONTROL_LINE_STATE = 0x22;


// USB PSTN table 3: Call management functional descriptor

/// USB PSTN call management functional descriptor
typedef struct qsb_pstn_call_management_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// Type of this descriptor (use \ref QSB_CDC_FUNC_DT_INTERFACE)
	uint8_t bDescriptorType;
    /// Subtype of this descriptor (use \ref QSB_CDC_FUNC_SUBTYPE_CALL_MANAGEMENT)
	uint8_t bDescriptorSubtype;
	/// The capabilities that this configuration supports
	uint8_t bmCapabilities;
	/// Interface number of the Data Class interface optionally used for call management
	uint8_t bDataInterface;
} __attribute__((packed)) qsb_pstn_call_management_desc;

// USB PSTN table 4: Abstract control management functional descriptor

/// USB PSTN Abstract control management functional descriptor
typedef struct qsb_cdc_acm_desc {
    /// Size of this descriptor, in bytes
	uint8_t bFunctionLength;
    /// Type of this descriptor (use \ref QSB_CDC_FUNC_DT_INTERFACE)
	uint8_t bDescriptorType;
    /// Subtype of this descriptor (use \ref QSB_CDC_FUNC_SUBTYPE_ACM)
	uint8_t bDescriptorSubtype;
	/// The capabilities that this configuration supports (QSB_ACM_CAP_xxx)
	uint8_t bmCapabilities;
} __attribute__((packed)) qsb_cdc_acm_desc;

// USB PSTN table 17: Line coding structure

/// USB PSTN line coding structure
typedef struct qsb_pstn_line_coding {
	/// Data terminal rate, in bits per second
	uint32_t dwDTERate;
	/// Stop bits (0: 1 stop bit; 1: 1.5 stop bits; 2: 2 stop bits)
	uint8_t bCharFormat;
	/// Parity (0: none; 1: odd; 2: even; 3: mark; 4: space)
	uint8_t bParityType;
	/// Data bits (5, 6, 7, 8 or 16)
	uint8_t bDataBits;
} __attribute__((packed)) qsb_pstn_line_coding;

/// USB PSTN char format
typedef enum qsb_pstn_line_coding_char_format {
	/// 1 stop bit
    QSB_PSTN_1_STOP_BITS     = 0,
	/// 1.5 stop bits
    QSB_PSTN_1_5_STOP_BITS   = 1,
	/// 2 stop bits
    QSB_PSTN_2_STOP_BITS     = 2
} qsb_pstn_line_coding_char_format;

/// USB PSTN parity type
typedef enum qsb_pstn_line_coding_parity_type {
	/// no parity
    QSB_PSTN_NO_PARITY       = 0,
	/// odd parity
    QSB_PSTN_ODD_PARITY      = 1,
	/// even parity
    QSB_PSTN_EVEN_PARITY     = 2,
	/// mark parity
    QSB_PSTN_MARK_PARITY     = 3,
	/// space parity
    QSB_PSTN_SPACE_PARITY    = 4
} qsb_pstn_line_coding_parity_type;

// USB PSTN table 30: Subclass specific notifications

/// PSTN notification code for serial state notification
static const uint8_t QSB_PSTN_NOTIF_SERIAL_STATE = 0x20;

// USB PSTN table 31: UART state bitmap values

/// USB PSTN UART state bit definitions
typedef enum {

	/// PSTN serial state bit: State of receiver carrier detection mechanism of device. This signal corresponds to V.24 signal 109 and RS-232 signal DCD
	QSB_PSTN_UART_STATE_RX_CARRIER = 0x01,
	/// PSTN serial state bit: State of transmission carrier. This signal corresponds to V.24 signal 106 and RS-232 signal DSR
	QSB_PSTN_UART_STATE_TX_CARRIER = 0x02,
	/// PSTN serial state bit: State of break detection mechanism of the device
	QSB_PSTN_UART_STATE_BREAK = 0x04,
	/// PSTN serial state bit: State of ring signal detection of the device
	QSB_PSTN_UART_STATE_RING_SIGNAL = 0x08,
	/// PSTN serial state bit: A framing error has occurred
	QSB_PSTN_UART_STATE_FRAMING = 0x10,
	/// PSTN serial state bit: A parity error has occurred
	QSB_PSTN_UART_STATE_PARITY = 0x20,
	/// PSTN serial state bit: Received data has been discarded due to overrun in the device
	QSB_PSTN_UART_STATE_OVERRUN = 0x40,

} qsb_pstn_uart_state_e;

// USB PSTN ch 6.5.4: Serial state notification structure

/// USB PSTN serial state notification structure
typedef struct qsb_pstn_serial_state_notif {
	/// Characteristics of notification (use \ref QSB_REQ_TYPE_IN `|` \ref QSB_REQ_TYPE_CLASS `|` \ref QSB_REQ_TYPE_INTERFACE)
	uint8_t bmRequestType;
	/// Notification code (use \ref QSB_PSTN_NOTIF_SERIAL_STATE)
	uint8_t bNotification;
	/// Value (0)
	uint16_t wValue;
	/// Index (interface number)
	uint16_t wIndex;
	/// Length of data (2)
	uint16_t wLength;
	/// Serial/UART state (see \ref qsb_pstn_uart_state_e)
	uint16_t wSerialState;
} __attribute__((packed)) qsb_pstn_serial_state_notif;


/// Functional descriptors for USB CDC control interface
typedef struct qsb_cdc_functional_descs {
	/// Header functional descriptor
	qsb_cdc_header_desc header;
	/// Call management functional descriptor
	qsb_pstn_call_management_desc call_mgmt;
	/// Abstract call management functional descriptor
	qsb_cdc_acm_desc acm;
	/// Union functional descriptor
	qsb_cdc_union_desc cdc_union;
} __attribute__((packed)) qsb_cdc_functional_descs;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif 
