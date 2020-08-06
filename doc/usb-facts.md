# What you need to know about USB and the STM32 USB peripheral 


## USB host and devices

In classical USB, there are fixed roles:

- The *host* (usually a computer) is the central instance on the USB bus. It initiates all communication. It configures all USB devices.
- The *device* (such a keyboard or flash drive plugged into your computer) responds to communication by the host.

As a device may not start a communication, the host will ask the device every millisecond if it has data to transmit.


## USB endpoints

Each USB device offers one or more *endpoints*. Endpoints are separately addressable buffers for communication. Each device must offer at least endpoint 0, a *control endpoint* for configuring the device. Control endpoints are used to exchange control messages; they are bidirectional. *Data endpoints* are used to transfer data. They are unidirectional only. They come in three flavors: bulk, isochronous and interrupt. For this project, only control endpoints and *bulk data endpoints* are relevant.

Bulk data endpoints basically implement a unidirectional data stream (aka as *pipe*). Data is transmitted in chunks of up to 64 bytes (for USB full speed). They have no concept of messages and therefore no concept of message boundaries.

In USB terminology, the direction of an endpoint is named from the host's perspective. Thus, *IN* always refers to transfers from a device to the host and *OUT* always refers to transfers from the host to a device.


## USB descriptor

Each USB device needs to provide descriptive information about itself called the USB descriptor. It describes the protocols (called device class / subclass) supported by the device, the possible configurations and the endpoints.

The host will initially read the USB descriptor and then activate the device when it's ready. For composite devices, it might not activate all of them.


## USB serial device

USB serial devices implement a USB CDC device with the subclass PSTN and are defined in [Class definition for Communication Devices 1.2](https://www.usb.org/document-library/class-definitions-communication-devices-12). They basically consist of a CDC configuration with three endpoints:
- Control endpoint for configuration requests (baud rate, DTR state etc.) and notifications (DCD state, errors)
- Data endpoint for USB to serial transmission
- Data endpoint for serial to USB transmission


## USB composite device

Composite devices implement several functions in a single device such as mass storage and serial device, or two serial devices. When plugged in, they usually appear on the computer (host) as two or more separate devices.

The device implements it by providing a USB descriptor with two or more *interface descriptions*, and each interface comprises several endpoints.

A USB serial device is always a USB composite device, even if it only implements a single serial device.


## USB peripheral in STM32 MCUs

The USB peripheral implements all low-level aspects of the USB protocol, in particular the transceiver incl. NRZI encoding/decoding, CRC generation/check and basic interaction with the host.

The interaction between the code running in the MCU and the USB peripheral is via the configuration and status registers, the USB interrupt and the PMA buffers.

It is essential to understand that the peripheral will always act immediately without executing any MCU code first. Therefore, the USB interrupt is triggered after the fact.

If the USB peripheral receives a packet from the host, it will store it into the PMA buffer, update status registers and trigger an interrupt. If the host polls the device for data, the USB peripheral will respond with the data that's already in the PMA buffer and then trigger an interrupt. If no data is ready, the peripheral will immediately answer with NAK.


## PMA buffers

The PMA buffers are a special static RAM area that is accessed both by the MCU code and the USB peripheral. The USB peripheral can only transmit from and receive to this memory area. Each endpoint has one or two dedicated areas in the PMA buffers.

For maximum performance, *double buffering* can be used, i.e. two areas in the PMA buffers for the same endpoint. They are then used alternatively so that one area can be used by the USB peripheral while the MCU code fills the other one. This project does not use double buffering, in particular because it is not supported by *libopencm3*.


## libopencm3

*libopencm3* implements major parts of the USB device protocol, in particular the communication regarding the USB descriptor, dispatching of messages to separate callbacks for each endpoint as well as the low-level configuration of USB registers.

With *libopencm3*, the USB descriptor can be provided as a easy to setup data structure; hardly any other code is needed.
