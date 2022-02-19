//
//  USB Serial
//
// Copyright (c) 2022 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Serial port class (for Linux).
//

#include "serial.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/termbits.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>

/**
 * Sets the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to set
 */
static inline void set_bits(unsigned int& flags, unsigned int bits) {
    flags |= bits;
}


/**
 * Clears the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to clear
 */
static inline void clear_bits(unsigned int& flags, unsigned int bits) {
    flags &= ~bits;
}


serial_port::serial_port() : _fd(-1) { }

void serial_port::open(const char* path, int bit_rate, int data_bits, bool with_parity) {
    int fd = ::open(path, O_RDWR | O_NOCTTY);
    if (fd == -1)
        throw serial_error("Unable to open serial port", errno);

    struct termios2 options;
    // tcgetattr(fd, &options);
    ioctl(fd, TCGETS2, &options);

    // Turn off all interactive / terminal features
    clear_bits(options.c_iflag, IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    set_bits(options.c_iflag, IGNPAR | IGNBRK);
    clear_bits(options.c_oflag, OPOST | ONLCR | OCRNL);
    set_bits(options.c_oflag, 0);
    clear_bits(options.c_cflag, PARENB | PARODD | CSTOPB | CSIZE | CBAUD);
    set_bits(options.c_cflag, CRTSCTS | CLOCAL | CREAD | BOTHER);
    clear_bits(options.c_lflag, ICANON | IEXTEN | ISIG | ECHO | ECHOE | ECHONL);
    set_bits(options.c_lflag, 0);
    
    set_bits(options.c_cflag, data_bits == 7 ? CS7 : CS8);
    if (with_parity)
        set_bits(options.c_cflag, PARENB);

    // Read returns if no character has been received in 10ms
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN]  = 0;
    
    options.c_ispeed = bit_rate;
    options.c_ospeed = bit_rate;
    
    // tcsetattr(fd, TCSANOW, &options);
    ioctl(fd, TCSETS2, &options);
    
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
