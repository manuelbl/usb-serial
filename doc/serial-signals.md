
# Data and Control Signals

The data and control signals are derived from the RS-232 standard. In its original incarnation, the standard used +12V and -12V as signal levels. This has evoled into TTL logic levels (+5V and 0V) and 3.3V logic levels (+3.3V and 0V). The hardware of the USB-to-serial adapter uses 3.3V logical levels. Input signals are 5V tolerant.

The standard is based around the idea that a computer is connected to a modem, the modem communicates to another modem, which in turn is connected to a computer. The RS-232 standard would apply to the connection between the computer and the modem. It uses the terms *data terminal equipment (DTE)* for the computer and *data circuit-terminating equipment (DCE)* for the modem.

For simplicity, this document talks about the computer (DTE) and the device (DCE).  The USB-to-serial adapter is seen as part in-between. It implements a number of signals:

## Data Signals

The data singals use 3.3V logic levels:

Data      | Voltage
----------|---------
0 (space) |   0V
1 (mark)  | 3.3V

The control signals adhere to the RS-232 standard (except for using logic level voltages):

Signal   | Direction | Purpose
---------|-----------|---------
TX       | Output    | Data transmission to device.
RX       | Input     | Data reception from device.


## Control Signals

All control signals are *active low*, i.e. 0V is asserted, 3.3V is deasserted:

Control    | Voltage
-----------|--------
Asserted   |   0V
Deasserted | 3.3V

The control signals adhere to the RS-232 standard, or rather its latest interpretation:


Signal | Direction | Purpose | Implementation | Default state
-------|-----------|---------|----------------|--------------
CTS       | Input   | Hardware handshake: device asserts signal to indicate it is ready to accept data.| Stops transmitting data (on TX signal) if not asserted. Once transmit buffer becomes full, NAK is signalled for OUT packages received from computer. | Asserted (using input with pull-down)
RTS (RTR) | Output  | Hardware handshake: adapter is ready to accept data (on RX signal). | When the receive buffer's fill level has reach the high-water mark, the signal is deasserted to indicate that the device should pause sending data. | Initially asserted
DTR       | Output  | *“Computer is ready.”* Usually used for custom purposes. | Output signal is controlled by computer with *SetControlLineState* request (bit D0). | Asserted
DSR       | Input   | *“Device is ready.”* Usually used for custom purposes. | Changes of the input signal are communicated to the computer using *SerialState* notification (bit *TxCarrier*). | Asserted (using input with pull-down)
DCD       | Input   | *“Data carrier detected.”* Usually used for custom purposes. | Changes of the input signal are communicated to computer using *SerialState* notification (bit *RxCarrier*). | Asserted (using input with pull-down)

Note that *RTS* has been redefined over time from being used for half-duxplex communication to serving for hardware flow control. The latter one is correctly called *RTR* for *ready to receive*. However, in most software and hardware products, it's still called *RTS*.

The *USB PSTN Devices Specification* specifies bit D1 in the *SetControlLineState* request for controlling *RTS* for half-duplex communication. As this adapter uses *RTS* as *RTR* for hardware flow control, bit D1 is disregarded.
