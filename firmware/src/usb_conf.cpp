/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB device configuration
 */

#include "common.h"
#include "hardware.h"
#include "usb_cdc.h"
#include "usb_conf.h"
#include "qsb_device.h"
#include "qsb_cdc.h"
#include <string.h>

#define USB_VID 0x1209
#define USB_PID 0x8048
#define USB_DEVICE_REL 0x0120

#define INTF_COMM 0 //  COMM must be immediately before DATA because of Associated Interface Descriptor.
#define INTF_DATA 1

#define USB_CONTROL_BUF_SIZE 256

static uint8_t usbd_control_buffer[USB_CONTROL_BUF_SIZE] __attribute__((aligned(4)));

static const char * const usb_strings[] = {
	"Codecrete",           //  USB Manufacturer
	"USB Serial",          //  USB Product
	qsb_serial_num,        //  Serial number
	"Virtual Serial Port", //  Interface assocation
	"USB Serial COMM 1",   //  Communication interface
	"USB Serial DATA 1",   //  Data interface
};

enum usb_strings_index
{ //  Index of USB strings.  Must sync with above, starts from 1.
	USB_STRINGS_MANUFACTURER_ID = 1,
	USB_STRINGS_PRODUCT_ID,
	USB_STRINGS_SERIAL_NUMBER_ID,
	USB_STRINGS_SERIAL_PORT_ID,
	USB_STRINGS_COMM_1_ID,
	USB_STRINGS_DATA_1_ID,
};

// Serial ACM interface
static const qsb_endpoint_desc comm_ep_1_desc[] = {
	{
		.bLength = QSB_DT_ENDPOINT_SIZE,
		.bDescriptorType = QSB_DT_ENDPOINT,
		.bEndpointAddress = COMM_IN_1,
		.bmAttributes = QSB_ENDPOINT_ATTR_INTERRUPT,
		.wMaxPacketSize = 16,
		.bInterval = 255,
		.extra = nullptr,
		.extralen = 0,
	}};

static const qsb_endpoint_desc data_ep_1_desc[] = {
	{
		.bLength = QSB_DT_ENDPOINT_SIZE,
		.bDescriptorType = QSB_DT_ENDPOINT,
		.bEndpointAddress = DATA_OUT_1,
		.bmAttributes = QSB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = CDCACM_PACKET_SIZE,
		.bInterval = 1,
		.extra = nullptr,
		.extralen = 0,
	},
	{
		.bLength = QSB_DT_ENDPOINT_SIZE,
		.bDescriptorType = QSB_DT_ENDPOINT,
		.bEndpointAddress = DATA_IN_1,
		.bmAttributes = QSB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = CDCACM_PACKET_SIZE,
		.bInterval = 1,
		.extra = nullptr,
		.extralen = 0,
	}};

static const qsb_cdc_functional_descs cdc_func_desc = {
	.header = {
		.bFunctionLength = sizeof(qsb_cdc_header_desc),
		.bDescriptorType = QSB_CDC_FUNC_DT_INTERFACE,
		.bDescriptorSubtype = QSB_CDC_FUNC_SUBTYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = { // see chapter 5.3.1 in PSTN120
		.bFunctionLength = sizeof(qsb_pstn_call_management_desc),
		.bDescriptorType = QSB_CDC_FUNC_DT_INTERFACE,
		.bDescriptorSubtype = QSB_CDC_FUNC_SUBTYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0, // no call management
		.bDataInterface = INTF_DATA,
	},
	.acm = { // see chapter 5.3.2 in PSTN120
		.bFunctionLength = sizeof(qsb_cdc_acm_desc),
		.bDescriptorType = QSB_CDC_FUNC_DT_INTERFACE,
		.bDescriptorSubtype = QSB_CDC_FUNC_SUBTYPE_ACM,
		.bmCapabilities = QSB_ACM_CAP_LINE_CODING,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(qsb_cdc_union_desc),
		.bDescriptorType = QSB_CDC_FUNC_DT_INTERFACE,
		.bDescriptorSubtype = QSB_CDC_FUNC_SUBTYPE_UNION,
		.bControlInterface = INTF_COMM,
		.bSubordinateInterface0 = INTF_DATA,
	}};

// CDC interfaces descriptors
static const qsb_interface_desc comm_if_1_desc[] = {
	{
		.bLength = QSB_DT_INTERFACE_SIZE,
		.bDescriptorType = QSB_DT_INTERFACE,
		.bInterfaceNumber = INTF_COMM,
		.bAlternateSetting = 0,
        .bNumEndpoints = QSB_ARRAY_SIZE(comm_ep_1_desc),
		.bInterfaceClass = QSB_CDC_INTF_CLASS_COMM,
		.bInterfaceSubClass = QSB_CDC_INTF_SUBCLASS_ACM,
		.bInterfaceProtocol = QSB_CDC_INTF_PROTOCOL_AT,
		.iInterface = USB_STRINGS_COMM_1_ID,
		.endpoint = comm_ep_1_desc,
		.extra = &cdc_func_desc,
		.extralen = sizeof(cdc_func_desc),
	}};

static const qsb_interface_desc data_if_1_desc[] = {
	{
		.bLength = QSB_DT_INTERFACE_SIZE,
		.bDescriptorType = QSB_DT_INTERFACE,
		.bInterfaceNumber = INTF_DATA,
		.bAlternateSetting = 0,
		.bNumEndpoints = QSB_ARRAY_SIZE(data_ep_1_desc),
		.bInterfaceClass = QSB_CDC_INTF_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = USB_STRINGS_DATA_1_ID,
		.endpoint = data_ep_1_desc,
		.extra = nullptr,
		.extralen = 0,
	}};

static const qsb_iface_assoc_desc assoc_1_desc = {
	.bLength = QSB_DT_INTERFACE_ASSOCIATION_SIZE,
	.bDescriptorType = QSB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = INTF_COMM,
	.bInterfaceCount = 2,
	.bFunctionClass = QSB_CDC_INTF_CLASS_COMM,
	.bFunctionSubClass = QSB_CDC_INTF_SUBCLASS_ACM,
	.bFunctionProtocol = QSB_CDC_INTF_PROTOCOL_AT,
	.iFunction = USB_STRINGS_SERIAL_PORT_ID,
};

// All interfaces
static const qsb_interface usb_interfaces[] = {
	{
		.cur_altsetting = nullptr,
		.num_altsetting = QSB_ARRAY_SIZE(comm_if_1_desc),
		.altsetting = comm_if_1_desc,  // Index of this array element must match with INTF_COMM
		.iface_assoc = &assoc_1_desc,  // Mandatory for composite device with multiple interfaces
	},
	{
		.cur_altsetting = nullptr,
		.num_altsetting = QSB_ARRAY_SIZE(data_if_1_desc),
		.altsetting = data_if_1_desc,  // Index of this array element must match with INTF_DATA
		.iface_assoc = nullptr,
	},
};

static const qsb_config_desc config_desc[] = {
	{
		.bLength = QSB_DT_CONFIGURATION_SIZE,
		.bDescriptorType = QSB_DT_CONFIGURATION,
		.wTotalLength = 0,
		.bNumInterfaces = QSB_ARRAY_SIZE(usb_interfaces),
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = QSB_CONFIG_ATTR_DEFAULT, // bus-powered
		.bMaxPower = 50,	// 100 mA
		.interface = usb_interfaces,
	}
};

// USB device descriptor
static const qsb_device_desc dev_desc = {
	.bLength = QSB_DT_DEVICE_SIZE,
	.bDescriptorType = QSB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = QSB_DEV_CLASS_MISCELLANEOUS,
	.bDeviceSubClass = QSB_DEV_SUBCLASS_MISC_COMMON,
	.bDeviceProtocol = QSB_DEV_PROTOCOL_INTF_ASSOC_DESC,
	.bMaxPacketSize0 = 16,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = USB_DEVICE_REL,
	.iManufacturer = USB_STRINGS_MANUFACTURER_ID,
	.iProduct = USB_STRINGS_PRODUCT_ID,
	.iSerialNumber = USB_STRINGS_SERIAL_NUMBER_ID,
	.bNumConfigurations = QSB_ARRAY_SIZE(config_desc),
};

qsb_device *usb_conf_init()
{
	return qsb_dev_init(qsb_port_fs, &dev_desc, config_desc,
					 usb_strings, QSB_ARRAY_SIZE(usb_strings),
					 usbd_control_buffer, sizeof(usbd_control_buffer));
}
