//
//  USB Serial
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Serial port class (for Windows).
//

#include "serial.hpp"
#include <windows.h>

serial_port::serial_port() : _handle(NULL) { }


void serial_port::open(const char* port, int bit_rate, int data_bits, bool with_parity) {
    char port_filename[255];
    strcpy_s(port_filename, "\\\\.\\");
    strcat_s(port_filename, port);

#pragma warning(suppress : 6054)
    HANDLE hComPort = CreateFile(port_filename,
        GENERIC_READ | GENERIC_WRITE,   // read/write
        0,                              // no Sharing
        NULL,                           // no Security
        OPEN_EXISTING,                  // open existing port only
        0,                              // non Overlapped I/O
        NULL);                          // NULL for Comm Devices

    if (hComPort == INVALID_HANDLE_VALUE)
        throw serial_error("Error opening serial port", GetLastError());

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    GetCommState(hComPort, &dcbSerialParams);

    dcbSerialParams.BaudRate = bit_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    BOOL result = SetCommState(hComPort, &dcbSerialParams);
    if (result == 0) {
        DWORD err = GetLastError();
        CloseHandle(hComPort);
        throw serial_error("Failed to set baud rate", err);
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    result = SetCommTimeouts(hComPort, &timeouts);
    if (result == 0) {
        DWORD err = GetLastError();
        CloseHandle(hComPort);
        throw serial_error("Failed to set comm timeouts", err);
    }

    _handle = hComPort;
}


void serial_port::close() {
    if (_handle == NULL)
        return;
    
    HANDLE handle = _handle;
    _handle = NULL;
    
    if (CloseHandle(handle) == 0)
        throw serial_error("Failed to close serial port", GetLastError());
}


void serial_port::transmit(const uint8_t* data, int data_len) {
    DWORD num_transmitted = 0;
    BOOL result = WriteFile(_handle, data, data_len, &num_transmitted, NULL);
    if (result == 0)
        throw serial_error("Failed to transmit data", GetLastError());
    if (num_transmitted != data_len)
        throw serial_error("Failed to transmit data");
}


int serial_port::receive(uint8_t* data, int data_len) {
    DWORD num_received = 0;
    BOOL result = ReadFile(_handle, data, data_len, &num_received, NULL);
    if (result == 0) {
        throw serial_error("Failed to receive data", GetLastError());
    }
    return num_received;
}


void serial_port::drain() {
    uint8_t buf[16] = { 0 };
    DWORD num_received = 0;
    BOOL result = FALSE;
    do {
        result = ReadFile(_handle, buf, sizeof(buf), &num_received, NULL);
    } while (result != 0 && num_received > 0);
}



// --- serial_error -------------

static std::string message_from_error(DWORD errnum) {
    LPTSTR msg = NULL;
    DWORD ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errnum,
        0,
        (LPTSTR)&msg,
        0,
        NULL);

    if (ret != 0) {
        std::string result(msg);
        LocalFree(msg);
        return result;
    }

    return "(unknown)";
}


serial_error::serial_error(const char* message, unsigned long errnum) noexcept
: _message(message), _code(errnum) {

    if (errnum != 0) {
        _message += ": ";
        _message += message_from_error(errnum);
    }
}

serial_error::~serial_error() noexcept { }

const char* serial_error::what() const noexcept {
    return _message.c_str();
}

long serial_error::error_code() {
    return _code;
}
