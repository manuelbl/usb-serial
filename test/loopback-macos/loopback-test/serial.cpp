//
//  USB Serial
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Serial port class (for macOS).
//

#include "serial.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <IOKit/serial/ioss.h>

/**
 * Sets the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to set
 */
static inline void set_bits(unsigned long& flags, unsigned long bits) {
    flags |= bits;
}


/**
 * Clears the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to clear
 */
static inline void clear_bits(unsigned long& flags, unsigned long bits) {
    flags &= ~bits;
}


serial_port::serial_port() : _fd(-1) { }

void serial_port::open(const char* path, int bit_rate, int data_bits, bool with_parity) {
    int fd = ::open(path, O_RDWR | O_NOCTTY);
    if (fd == -1)
        throw serial_error("Unable to open serial port", errno);

    struct termios options;
    tcgetattr(fd, &options);

    // Turn off all interactive / terminal features
    clear_bits(options.c_iflag, INLCR | ICRNL);
    set_bits(options.c_iflag, IGNPAR | IGNBRK);
    clear_bits(options.c_oflag, OPOST | ONLCR | OCRNL);
    set_bits(options.c_oflag, 0);
    clear_bits(options.c_cflag, PARENB | PARODD | CSTOPB | CSIZE);
    set_bits(options.c_cflag, CRTSCTS | CLOCAL | CREAD);
    clear_bits(options.c_lflag, ICANON | IEXTEN | ISIG | ECHO | ECHOE | ECHONL);
    set_bits(options.c_lflag, 0);
    
    set_bits(options.c_cflag, data_bits == 7 ? CS7 : CS8);
    if (with_parity)
        set_bits(options.c_cflag, PARENB);

    // Read returns if no character has been received in 10ms
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN]  = 0;
    
    tcsetattr(fd, TCSANOW, &options);

    // Use special call to set custom bit rates
    speed_t speed = bit_rate;
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        ::close(fd);
        throw serial_error("Cannot set bit rate", errno);
    }
    
    _fd = fd;
}

void serial_port::close() {
    if (_fd == -1)
        return;
    
    int fd = _fd;
    _fd = -1;
    
    if (::close(fd) != 0)
        throw serial_error("Failed to close serial port", errno);
}


void serial_port::transmit(const uint8_t* data, int data_len) {
    size_t res = ::write(_fd, data, data_len);
    if (res == -1)
        throw serial_error("Failed to transmit data", errno);
    if (res != data_len)
        throw serial_error("Failed to transmit data");
}

int serial_port::receive(uint8_t* data, int data_len) {
    size_t res = ::read(_fd, data, data_len);
    if (res == -1)
        throw serial_error("Failed to receive data", errno);
    return (int)res;
}

void serial_port::drain() {
    uint8_t buf[16];
    ssize_t k;
    do {
        k = ::read(_fd, buf, sizeof(buf));
    } while (k > 0);
    if (k == -1)
        throw serial_error("Failed to drain serial port", errno);
}



// --- serial_error -------------


serial_error::serial_error(const char* message, int errnum) noexcept
: _message(message), _code(errnum) {

    if (errnum != 0) {
        char msgbuf[256] = { 0 };
        if (strerror_r(errnum, msgbuf, sizeof(msgbuf)) == 0) {
            _message += ": ";
            _message += msgbuf;
        }
    }
}

serial_error::~serial_error() noexcept { }

const char* serial_error::what() const noexcept {
    return _message.c_str();
}

long serial_error::error_code() {
    return _code;
}
