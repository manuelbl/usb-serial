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

serial_port::serial_port() : _hComPort(NULL), _hEvent(NULL) { }

serial_port::~serial_port() {
    if (_hEvent != NULL)
        CloseHandle(_hEvent);
}

void serial_port::open(const char* port, int bit_rate, int data_bits, bool with_parity) {
    char port_filename[255];
    strcpy_s(port_filename, "\\\\.\\");
    strcat_s(port_filename, port);

#pragma warning(suppress : 6054)
    HANDLE hComPort = CreateFile(port_filename,
        GENERIC_READ | GENERIC_WRITE,   // read/write
        0,                              // no sharing
        NULL,                           // no security
        OPEN_EXISTING,                  // open existing port only
        FILE_FLAG_OVERLAPPED,           // overlapped I/O
        NULL);                          // NULL for comm devices

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
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    result = SetCommTimeouts(hComPort, &timeouts);
    if (result == 0) {
        DWORD err = GetLastError();
        CloseHandle(hComPort);
        throw serial_error("Failed to set comm timeouts", err);
    }

    _hComPort = hComPort;
}


void serial_port::close() {
    if (_hComPort == NULL)
        return;
    
    HANDLE handle = _hComPort;
    _hComPort = NULL;
    
    if (CloseHandle(handle) == 0)
        throw serial_error("Failed to close serial port", GetLastError());
}


void serial_port::transmit(const uint8_t* data, int data_len) {
    if (_hEvent == NULL) {
        _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (_hEvent == NULL)
            throw serial_error("Failed to create event object", GetLastError());
    }

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = _hEvent;
    BOOL result = WriteFile(_hComPort, data, data_len, NULL, &overlapped);
    if (result == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
            throw serial_error("Failed to start transmitting data", err);
    }

    DWORD num_transferred = 0;
    result = GetOverlappedResultEx(_hComPort, &overlapped, &num_transferred, 100, FALSE);
    if (result == 0)
        throw serial_error("Failed to transmit data", GetLastError());
    if (overlapped.InternalHigh != data_len)
        throw serial_error("Failed to transmit data");
}


int serial_port::receive(uint8_t* data, int data_len) {
    if (_hEvent == NULL) {
        _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (_hEvent == NULL)
            throw serial_error("Failed to create event object", GetLastError());
    }

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = _hEvent;
    BOOL result = ReadFile(_hComPort, data, data_len, NULL, &overlapped);
    if (result == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
            throw serial_error("Failed to start receiving data", err);
    }

    DWORD num_transferred = 0;
    result = GetOverlappedResultEx(_hComPort, &overlapped, &num_transferred, 100, FALSE);
    if (result == 0)
        throw serial_error("Failed to receive data", GetLastError());
    return num_transferred;
}


void serial_port::drain() {
    uint8_t buf[16] = { 0 };
    while (true) {
        int num_received = receive(buf, sizeof(buf));
        if (num_received == 0)
            break;
    };
}



// --- serial_error -------------

static std::string message_from_error(DWORD errnum) {
    LPTSTR msg = NULL;
    DWORD ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errnum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
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
