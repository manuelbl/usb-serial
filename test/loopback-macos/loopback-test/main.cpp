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
// Comand line syntax: loopback-test send_port recv_port [ bit_rate [ num_bytes ]
//
// Specify the same port for send_port and recv_port for single port configuration.
//

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


/**
 * Pseudo Random Number Generator
 */
struct prng {
    /**
     * Constructs a new instance
     * @param init initial value
     */
    prng(uint32_t init);
    /**
     * Returns the next pseudo random value
     * @return pseudo random value
     */
    uint32_t next();
    /**
     * Fills the buffer with pseudo random data
     * @param buf buffer receiving the random data
     * @param len length of the buffer (in bytes)
     */
    void fill(uint8_t* buf, size_t len);

private:
    uint32_t state;
    int nbytes;
    uint32_t bits;
};


prng::prng(uint32_t init) : state(init), nbytes(0), bits(0) { }


uint32_t prng::next() {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}


void prng::fill(uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (nbytes == 0) {
            bits = next();
            nbytes = 4;
        }
        buf[i] = bits;
        bits >>= 8;
        nbytes--;
    }
}


static const char* send_port;
static const char* recv_port;
static int send_fd;
static int recv_fd;
static int num_bytes = 300000;
static int bit_rate = 921600;
static volatile bool test_cancelled = false;


/**
 * Checks the program arguments
 * @param argc number of arguments
 * @param argv argument array
 * @return 0 on success, other value on error
 */
static int check_usage(int argc, const char* argv[]);

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
 * Main function
 * @param argc number of arguments
 * @param argv argument array
 */
int main(int argc, const char * argv[]) {
    setlocale(LC_NUMERIC, "en_US");

    if (check_usage(argc, argv) != 0)
        exit(1);
    
    if (open_ports() != 0)
        exit(2);
    
    // Run send function in separate thread
    pthread_t t;
    pthread_create(&t, NULL, send, NULL);
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    recv();

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    double duration = end_time.tv_sec - start_time.tv_sec;
    duration += (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    close_ports();

    if (!test_cancelled) {
        printf("Successfully sent %'d bytes in %.1fs\n", num_bytes, duration);
        printf("Gross bit rate: %'d bps\n", bit_rate);
        int br = (int)(num_bytes * 8 / duration);
        printf("Net bit rate:   %'d bps\n", br);
        printf("Overhead: %.1f%%\n", bit_rate * 80.0 / br - 100);
    }
    
    return test_cancelled ? 3 : 0;
}


int check_usage(int argc, const char* argv[]) {
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s send_port [ recv_port [ bit_rate [ num_bytes ] ] ]", argv[0]);
        return 1;
    }
    
    send_port = recv_port = argv[1];
    if (argc >= 3)
        recv_port = argv[2];
    if (argc >= 4)
        bit_rate = atoi(argv[3]);
    if (argc >= 5)
        num_bytes = atoi(argv[4]);
        
    if (bit_rate < 1200 || bit_rate > 99999999) {
        fprintf(stderr, "Bit rate %d out of range (1200 .. 99,999,999)\n", bit_rate);
        return 2;
    }
    if (num_bytes < 1) {
        fprintf(stderr, "Number of bytes %d must be a positive number\n", num_bytes);
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
            fprintf(stderr, "No more data from %s after %ld bytes\n", recv_port, n);
            test_cancelled = true;
            return;
        }
        
        prandom.fill(expected, k);
        if (memcmp(buf, expected, k) != 0) {
            fprintf(stderr, "Invalid data at pos %ld\n", n);
            test_cancelled = true;
            return;
        }
        n += k;
    }
}


int open_ports() {
    send_fd = open_port(send_port);
    if (send_fd == -1)
        return -1;
    
    if (strcmp(send_port, recv_port) == 0) {
        recv_fd = send_fd;
        
    } else {
        recv_fd = open_port(recv_port);
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
    set_bits(options.c_cflag, CRTSCTS | CLOCAL | CREAD | CS8 );
    clear_bits(options.c_lflag, ICANON | IEXTEN | ISIG | ECHO | ECHOE | ECHONL);
    set_bits(options.c_lflag, 0);

    // Read returns if no character has been received in 10ms
    options.c_cc[VTIME] = 10;
    options.c_cc[VMIN]  = 0;
    
    tcsetattr(fd, TCSANOW, &options);

    // Use special call to set custom bit rates
    speed_t speed = bit_rate;
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        fprintf(stderr, "Cannot set bit rate to %ld\n", speed);
        close(fd);
        return -1;
    }

    return fd;
}
