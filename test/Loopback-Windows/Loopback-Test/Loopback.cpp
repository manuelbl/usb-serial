//
//  USB Serial
//
// Copyright (c) 2020 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Loopback test (for Windows)
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
#include "prng.hpp"
#include "serial.hpp"
#include <algorithm>
#include <iomanip>
#include <thread>

using namespace std::chrono;

static constexpr uint32_t PRNG_INIT = 0x7b;

// parsed command line arguments
static std::string send_port_path;
static std::string recv_port_path;
static int num_bytes;
static int bit_rate;
static int data_bits;
static bool with_parity;
static int rx_delay;

static serial_port send_port;
static serial_port recv_port;
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
 * Close  the serial port(s)
 */
static void close_ports();

/**
 * Sends pseudo random data to the serial port
 */
static void send();

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
 * Prints a hex dump of the specified buffer.
 * @param title Title to print at start of line
 * @param buf buffer start
 * @para buf_len buffer length
 */
static void hex_dump(const char* title, const uint8_t* buf, size_t buf_len);


/**
 * Main function
 * @param argc number of arguments
 * @param argv argument array
 */
int main(int argc, char* argv[]) {
    setlocale(LC_NUMERIC, "en_US");

    if (check_usage(argc, argv) != 0)
        exit(1);

    try {
        open_ports();

        // Run send function in separate thread
        std::thread sender(send);

        if (rx_delay != 0)
            std::this_thread::sleep_for(seconds(rx_delay));
        else
            std::this_thread::sleep_for(milliseconds(100));

        // start time
        time_point<high_resolution_clock> start_time = high_resolution_clock::now();

        // receive data
        recv();

        // end time
        time_point<high_resolution_clock> end_time = high_resolution_clock::now();
        double duration = static_cast<double>(duration_cast<seconds>(end_time - start_time).count());

        sender.join();
        close_ports();

        if (!test_cancelled) {
            int br = (int)(num_bytes * data_bits / duration);
            double expected_net_rate = bit_rate * data_bits / (double)(data_bits + (with_parity ? 1 : 0) + 2);
            printf("Successfully sent %d bytes in %.1fs\n", num_bytes, duration);
            printf("Gross bit rate: %d bps\n", bit_rate);
            printf("Net bit rate:   %d bps\n", br);
            printf("Overhead: %.1f%%\n", expected_net_rate * 100.0 / br - 100);
        }

    }
    catch (serial_error& error) {
        std::cerr << error.what() << std::endl;
        return 2;
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
        options.parse_positional({ "tx-port", "rx-port" });
        auto result = options.parse(argc, argv);

        if (result.count("tx-port") == 0)
            throw cxxopts::OptionParseException("'tx-port' not specified");

        if (result.count("help") != 0) {
            std::cout << options.help() << std::endl;
            return 2;
        }

        bit_rate = result["bitrate"].as<int>();
        bit_rate = std::min(std::max(bit_rate, 1200), 99999999);
        num_bytes = result["numbytes"].as<int>();
        num_bytes = std::min(std::max(num_bytes, 1), 1000000000);
        data_bits = result["databits"].as<int>();
        send_port_path = result["tx-port"].as<std::string>();
        rx_delay = result["rx-sleep"].as<int>();
        with_parity = result.count("parity") > 0;
        if (with_parity)
            data_bits = std::min(std::max(data_bits, 7), 8);
        else
            data_bits = 8;
        if (result.count("rx-port") > 0)
            recv_port_path = result["rx-port"].as<std::string>();
        else
            recv_port_path = send_port_path;

    }
    catch (const cxxopts::OptionException& e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        return 3;
    }

    return 0;
}


void send() {
    prng prandom(PRNG_INIT);
    uint8_t buf[128];

    try {

        int n = num_bytes;
        while (n > 0 && !test_cancelled) {
            int m = std::min((int)sizeof(buf), n);
            prandom.fill(buf, m);
            if (data_bits == 7)
                clear_high_bit(buf, m);
            send_port.transmit(buf, m);
            n -= m;
        }
    }
    catch (serial_error& error) {
        std::cerr << error.what() << std::endl;
        test_cancelled = true;
    }
}


void recv() {
    uint8_t buf[128];
    uint8_t expected[128];
    prng prandom(PRNG_INIT);

    try {

        int n = 0;
        while (n < num_bytes && !test_cancelled) {
            int k = recv_port.receive(buf, sizeof(buf));
            if (k == 0) {
                std::cerr << "No more data from " << recv_port_path << " after " << n << " bytes" << std::endl;
                test_cancelled = true;
                return;
            }

            prandom.fill(expected, k);
            if (data_bits == 7)
                clear_high_bit(expected, k);
            if (memcmp(buf, expected, k) != 0) {
                std::cerr << "Invalid data at pos " << n << std::endl;
                hex_dump("Expected: ", expected, k);
                hex_dump("Received: ", buf, k);
                test_cancelled = true;
                return;
            }
            n += k;
        }

    }
    catch (serial_error& error) {
        std::cerr << error.what() << std::endl;
        test_cancelled = true;
    }
}


int open_ports() {
    send_port.open(send_port_path.c_str(), bit_rate, data_bits, with_parity);

    if (send_port_path == recv_port_path) {
        recv_port = send_port;

    }
    else {
        recv_port.open(recv_port_path.c_str(), bit_rate, data_bits, with_parity);
    }

    recv_port.drain();
    return 0;
}


void close_ports() {
    recv_port.drain();
    send_port.close();
    if (recv_port_path != send_port_path)
        recv_port.close();
}


void clear_high_bit(uint8_t* buf, size_t buf_len) {
    for (int i = 0; i < buf_len; i++)
        buf[i] &= 0x7f;
}


void hex_dump(const char* title, const uint8_t* buf, size_t buf_len)
{
    std::cerr << title;

    for (size_t i = 0; i < buf_len; i++) {
        if (i > 0)
            std::cerr << ' ';
        std::cerr << std::hex << std::setfill('0') << std::setw(2) << (int)buf[i];
    }

    std::cerr << std::endl;
}
