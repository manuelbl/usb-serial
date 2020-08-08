# Throttler

 Simple test fixture for testing hardware flow control (CTS/RTS).
 
 This firmware passes data received on UART1 RX to UART2 TX and vice versa 
 (UART2 RX to UART1 TX). At the interface, it works with 115,200 bps.
 Internally, it throttles the speed to 2 bytes/ms (about about 20,000 bps).
 
