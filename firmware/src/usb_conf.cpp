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
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
#include <string.h>

#define USB_VID 0x1209
#define USB_PID 0x8048
#define USB_DEVICE_REL 0x0100

#define INTF_COMM 0 //  COMM must be immediately before DATA because of Associated Interface Descriptor.
#define INTF_DATA 1

#define USB_CONTROL_BUF_SIZE 256

static uint8_t usbd_control_buffer[USB_CONTROL_BUF_SIZE] __attribute__((aligned(2)));

static char serial_number[USB_SERIAL_NUM_LENGTH + 1];

static const char *usb_strings[] = {
	"Codecrete",           //  USB Manufacturer
	"USB Serial",          //  USB Product
	serial_number,         //  Serial number
	"Virtual Serial Port", //  Interface assocation
	"USB Serial COMM 1",   //  Communication interface
	"USB Serial DATA 1",   //  Data interface
};

#define MSC_VENDOR_ID "CODECRET"         //  Max 8 chars
#define MSC_PRODUCT_ID "USB Serial"      //  Max 16 chars
#define MSC_PRODUCT_REVISION_LEVEL "1.0" //  Max 4 chars
#define USB_CLASS_MISCELLANEOUS 0xef

enum usb_strings_index
{ //  Index of USB strings.  Must sync with above, starts from 1.
	USB_STRINGS_MANUFACTURER_ID = 1,
	USB_STRINGS_PRODUCT_ID,
	USB_STRINGS_SERIAL_NUMBER_ID,
	USB_STRINGS_SERIAL_PORT_ID,
	USB_STRINGS_COMM_1_ID,
	USB_STRINGS_DATA_1_ID,
};

// USB device descriptor
static const struct usb_device_descriptor dev_desc = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0xEF, // miscellaneous device
	.bDeviceSubClass = 2, // common class
	.bDeviceProtocol = 1, // interface association
	.bMaxPacketSize0 = 64,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = USB_DEVICE_REL,
	.iManufacturer = USB_STRINGS_MANUFACTURER_ID,
	.iProduct = USB_STRINGS_PRODUCT_ID,
	.iSerialNumber = USB_STRINGS_SERIAL_NUMBER_ID,
	.bNumConfigurations = 1,
};

// Serial ACM interface
static const struct usb_endpoint_descriptor comm_ep_1_desc[] = {
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = COMM_IN_1,
		.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
		.wMaxPacketSize = 16,
		.bInterval = 255,
		.extra = nullptr,
		.extralen = 0,
	}};

static const struct usb_endpoint_descriptor data_ep_1_desc[] = {
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = DATA_OUT_1,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = CDCACM_PACKET_SIZE,
		.bInterval = 1,
		.extra = nullptr,
		.extralen = 0,
	},
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = DATA_IN_1,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = CDCACM_PACKET_SIZE,
		.bInterval = 1,
		.extra = nullptr,
		.extralen = 0,
	}};

static const struct
{
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_desc = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = INTF_DATA,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 2,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = INTF_COMM,
		.bSubordinateInterface0 = INTF_DATA,
	}};

// CDC interfaces descriptors
static const struct usb_interface_descriptor comm_if_1_desc[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = INTF_COMM,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = USB_CLASS_CDC,
		.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
		.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
		.iInterface = USB_STRINGS_COMM_1_ID,
		.endpoint = comm_ep_1_desc,
		.extra = &cdcacm_desc,
		.extralen = sizeof(cdcacm_desc),
	}};

static const struct usb_interface_descriptor data_if_1_desc[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = INTF_DATA,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = USB_STRINGS_DATA_1_ID,
		.endpoint = data_ep_1_desc,
		.extra = nullptr,
		.extralen = 0,
	}};

static const struct usb_iface_assoc_descriptor assoc_1_desc = {
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = INTF_COMM,
	.bInterfaceCount = 2,
	.bFunctionClass = USB_CLASS_CDC,
	.bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol = USB_CDC_PROTOCOL_AT,
	.iFunction = USB_STRINGS_SERIAL_PORT_ID,
};

// All interfaces
static const struct usb_interface usb_interfaces[] = {
	{
		.cur_altsetting = nullptr,
		.num_altsetting = 1,
		.iface_assoc = &assoc_1_desc,  // Mandatory for composite device with multiple interfaces
		.altsetting = comm_if_1_desc,  // Index of this array element must match with INTF_COMM
	},
	{
		.cur_altsetting = nullptr,
		.num_altsetting = 1,
		.iface_assoc = nullptr,
		.altsetting = data_if_1_desc,  // Index of this array element must match with INTF_DATA
	},
};

static const struct usb_config_descriptor config_desc = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = sizeof(usb_interfaces) / sizeof(usb_interfaces[0]),
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80, // Bus-powered, i.e. it draws power from USB bus
	.bMaxPower = 0xfa,	// 500 mA
	.interface = usb_interfaces,
};

usbd_device *usb_conf_init()
{
	return usbd_init(&USB_DRIVER, &dev_desc, &config_desc,
					 usb_strings, sizeof(usb_strings) / sizeof(usb_strings[0]),
					 usbd_control_buffer, sizeof(usbd_control_buffer));
}

void usb_set_serial_number(const char *serial)
{
	strncpy(serial_number, serial, USB_SERIAL_NUM_LENGTH);
	serial_number[USB_SERIAL_NUM_LENGTH] = '\0';
}
