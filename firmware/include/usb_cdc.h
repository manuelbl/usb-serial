/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB CDC Implementation
 */

#pragma once

#include "qsb_device.h"

/// Maximum USB packet size
#define CDCACM_PACKET_SIZE 64

/// Global USB device instance
extern qsb_device *usb_device;

/// Initializes the USB CDC device
void usb_cdc_init();

/// Polls the USB CDC device for new events
void usb_cdc_poll();

/***
 * @brief Gets if the device is connected to a host
 * 
 * This function returns `true` only if a USB host is connected
 * and has fully configured the device, i.e. a valid USB
 * communication has been setup.
 * 
 * @return `true` if it is connected, `false, otherwise
 */
bool usb_cdc_is_connected();
