//
//  USB Serial
//
// Copyright (c) 2020 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test
//
// Pseudo random data is sent to and received from serial port and compared.
//
// Either a single or two serial ports are used:
// - Single port: TX is expected to be wired to RX
// - Two ports: TX of the first serial port is expected to be wired to RX of the second one.
//
// Comand line syntax: loopback-test [ OPTIONS... ] tx-port [ rx-port ]
//
// Specify the same port for tx-port and rx-port for single port configuration.
//

#include "cxxopts.hpp"
#include "prng.h"
#include <algorithm>
#include <fcntl.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>


#define PRNG_INIT 0x7b


static std::string send_port;
static std::string recv_port;
static int send_fd;
static int recv_fd;
static int num_bytes;
static int bit_rate;
static int data_bits;
static bool with_parity;
static int rx_delay;
static volatile bool test_cancelled = false;


/**
 * Checks the program arguments
 * @param argc number of arguments
 * @param argv argument array
 * @return 0 on success, other value on error
 */
static int check_usage(int argc, char* argv[]);

/**
 * Open the serial port(s) specified on the command line
 *
 * @return 0 on success, other value on error
 */
static int open_ports();

/**
 * Drain pending input data for specified port.
 * @param fd serial port file descriptor
 */
static void drain_port(int fd);

/**
 * Close  the serial port(s)
 */
static void close_ports();

/**
 * Opens the specified serial port
 * @param port the path to the serial port
 * @return file descriptor, or -1 on error
 */
static int open_port(const char* port);

/**
 * Sends pseudo random data to the serial port
 * @param ignore ignored parameter
 * @return always NULL
 */
static void* send(void* ignore);

/**
 * Receives data from the serial port and compares it with the expected data
 */
static void recv();

/**
 * Clears the high bit of each byte in the buffer.
 * @param buf buffer to be modified
 * @param buf_len length of buffer (in bytes)
 */
static void clear_high_bit(uint8_t* buf, size_t buf_len);

/**
 * Main function
 * @param argc number of arguments
 * @param argv argument array
 */
int main(int argc, char * argv[]) {
    setlocale(LC_NUMERIC, "en_US");
    
    if (check_usage(argc, argv) != 0)
        exit(1);
    
    if (open_ports() != 0)
        exit(2);
    
    // Run send function in separate thread
    pthread_t t;
    pthread_create(&t, NULL, send, NULL);
    
    if (rx_delay != 0)
        sleep(rx_delay);
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    recv();

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    double duration = end_time.tv_sec - start_time.tv_sec;
    duration += (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    close_ports();

    if (!test_cancelled) {
        int br = (int)(num_bytes * data_bits / duration);
        double expected_net_rate = bit_rate * data_bits / (double)(data_bits + (with_parity ? 1 : 0) + 2);
        printf("Successfully sent %'d bytes in %.1fs\n", num_bytes, duration);
        printf("Gross bit rate: %'d bps\n", bit_rate);
        printf("Net bit rate:   %'d bps\n", br);
        printf("Overhead: %.1f%%\n", expected_net_rate * 100.0 / br - 100);
    }
    
    return test_cancelled ? 3 : 0;
}


int check_usage(int argc, char* argv[]) {
    
    cxxopts::Options options("loopback", "Serial port loopback test");

    options.add_options()
        ("t,tx-port", "Serial port for transmission", cxxopts::value<std::string>())
        ("r,rx-port", "Serial port for reception (default: same as tx-port)", cxxopts::value<std::string>())
        ("n,numbytes", "Number of bytes to transmit", cxxopts::value<int>()->default_value("300000"))
        ("b,bitrate", "Bit rate (1200 .. 99,999,999 bps)", cxxopts::value<int>()->default_value("921600"))
        ("p,parity", "Enable parity bit")
        ("d,databits", "Data bits (7 or 8)", cxxopts::value<int>()->default_value("8"))
        ("s,rx-sleep", "Sleep before reception (in s)", cxxopts::value<int>()->default_value("0"))
        ("h,help", "Show usage");
    options.positional_help("tx-port [ rx-port ]").show_positional_help();
    
    try {
        options.parse_positional({"tx-port", "rx-port"});
        auto result = options.parse(argc, argv);
    
        if (result.count("help")) {
          std::cout << options.help() << std::endl;
          return 2;
        }

        bit_rate = result["bitrate"].as<int>();
        bit_rate = std::min(std::max(bit_rate, 1200), 99999999);
        num_bytes = result["numbytes"].as<int>();
        num_bytes = std::min(std::max(num_bytes, 1), 1000000000);
        data_bits = result["databits"].as<int>();
        send_port = result["tx-port"].as<std::string>();
        rx_delay = result["rx-sleep"].as<int>();
        with_parity = result.count("parity") > 0;
        if (with_parity)
            data_bits = std::min(std::max(data_bits, 7), 8);
        else
            data_bits = 8;
        if (result.count("rx-port") > 0)
            recv_port = result["rx-port"].as<std::string>();
        else
            recv_port = send_port;

    } catch (const cxxopts::OptionException& e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        return 3;
    }

    return 0;
}


void* send(void* ignore) {
    prng prandom(PRNG_INIT);
    uint8_t buf[128];

    size_t n = num_bytes;
    while (n > 0 && !test_cancelled) {
        size_t m = std::min(sizeof(buf), n);
        prandom.fill(buf, m);
        if (data_bits == 7)
            clear_high_bit(buf, m);
        size_t k = write(send_fd, buf, m);
        if (k != m) {
            perror("Write failed");
            test_cancelled = true;
            return NULL;
        }
        n -= m;
    }
    
    tcdrain(send_fd);
    return NULL;
}


void recv() {
    uint8_t buf[128];
    uint8_t expected[128];
    prng prandom(PRNG_INIT);
    size_t n = 0;
    
    while (n < num_bytes && !test_cancelled) {
        ssize_t k = read(recv_fd, buf, sizeof(buf));
        if (k == 0) {
            std::cerr << "No more data from " << recv_port << " after " << n << " bytes\n" << std::endl;
            test_cancelled = true;
            return;
        }
        
        prandom.fill(expected, k);
        if (data_bits == 7)
            clear_high_bit(expected, k);
        if (memcmp(buf, expected, k) != 0) {
            std::cerr << "Invalid data at pos " << n << std::endl;
            test_cancelled = true;
            return;
        }
        n += k;
    }
}


int open_ports() {
    send_fd = open_port(send_port.c_str());
    if (send_fd == -1)
        return -1;
    
    if (send_port == recv_port) {
        recv_fd = send_fd;
        
    } else {
        recv_fd = open_port(recv_port.c_str());
        if (recv_fd == -1) {
            close(send_fd);
            return -1;
        }
    }

    drain_port(recv_fd);
    return 0;
}


void drain_port(int fd) {
    uint8_t buf[16];
    ssize_t k;
    do {
        k = read(recv_fd, buf, sizeof(buf));
    } while (k > 0);
}


void close_ports() {
    drain_port(recv_fd);
    
    close(send_fd);
    if (send_fd != recv_fd)
        close(recv_fd);
}


/**
 * Sets the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to set
 */
static void set_bits(unsigned long& flags, unsigned long bits) {
    flags |= bits;
}


/**
 * Clears the specified bits in the specified flags value.
 * @param flags flags value
 * @param bits bits to clear
 */
static void clear_bits(unsigned long& flags, unsigned long bits) {
    flags &= ~bits;
}


int open_port(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("Unable to open serial port");
        return fd;
    }

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
        std::cerr << "Cannot set bit rate to " << speed << std::endl;
        close(fd);
        return -1;
    }

    return fd;
}

void clear_high_bit(uint8_t* buf, size_t buf_len) {
    for (int i = 0; i < buf_len; i++)
        buf[i] &= 0x7f;
}
