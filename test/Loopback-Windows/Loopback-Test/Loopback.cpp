#include <algorithm>
#include <tchar.h>
#include <windows.h>
#undef min
#include <stdint.h>
#include <stdio.h>
#include <shellapi.h>


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


static LPWSTR send_port;
static LPWSTR recv_port;
static HANDLE send_hdl;
static HANDLE recv_hdl;
static int num_bytes = 300000;
static int bit_rate = 921600;
static volatile bool test_cancelled = false;

/**
 * Checks the program arguments
 * @param argc number of arguments
 * @param argv argument array
 * @return 0 on success, other value on error
 */
static int check_usage(int argc, LPWSTR* argv);

/**
 * Open the serial port(s) specified on the command line
 *
 * @return 0 on success, other value on error
 */
static int open_ports();

/**
 * Drain pending input data for specified port.
 * @param hdl serial port handle
 */
static void drain_port(HANDLE hdl);

/**
 * Close  the serial port(s)
 */
static void close_ports();

/**
 * Opens the specified serial port
 * @param port the path to the serial port
 * @return serial port handle, or INVALID_HANDLE_VALUE on error
 */
static HANDLE open_com_port(LPWSTR port);

/**
 * Sends pseudo random data to the serial port
 * @param param ignored parameter
 * @return always 0
 */
DWORD __stdcall send(void* param);

/**
 * Receives data from the serial port and compares it with the expected data
 */
static void recv();


int __cdecl main(int argc, char* argv[]) {
    //setlocale(LC_NUMERIC, "en_US");

    int nArgs;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (check_usage(nArgs, szArglist) != 0)
        exit(1);

    if (open_ports() != 0)
        exit(2);

    // Run send function in separate thread
    HANDLE hThread = CreateThread(NULL, 0, send, NULL, 0, NULL);

    ULONGLONG start_time = GetTickCount64();

    recv();

    ULONGLONG end_time = GetTickCount64();
    double duration = (end_time - start_time) / 1000.0;

    WaitForSingleObject(hThread, INFINITE);

    close_ports();

    if (!test_cancelled) {
        printf("Successfully sent %d bytes in %.1fs\n", num_bytes, duration);
        printf("Gross bit rate: %d bps\n", bit_rate);
        int br = (int)(num_bytes * 8.0 / duration);
        printf("Net bit rate:   %d bps\n", br);
        printf("Overhead: %.1f%%\n", bit_rate * 80.0 / br - 100);
    }

    return test_cancelled ? 3 : 0;
}


int check_usage(int argc, LPWSTR* argv) {
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %ls send_port [ recv_port [ bit_rate [ num_bytes ] ] ]", argv[0]);
        return 1;
    }

    send_port = recv_port = argv[1];
    if (argc >= 3)
        recv_port = argv[2];
    if (argc >= 4)
        bit_rate = _wtoi(argv[3]);
    if (argc >= 5)
        num_bytes = _wtoi(argv[4]);

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

DWORD __stdcall send(void* param) {
    prng prandom(PRNG_INIT);
    uint8_t buf[128];

    size_t n = num_bytes;
    while (n > 0 && !test_cancelled) {
        size_t m = std::min(sizeof(buf), n);
        prandom.fill(buf, m);
        DWORD k;
        BOOL result = WriteFile(send_hdl, buf, m, &k, NULL);
        if (result == FALSE || k != m) {
            perror("Write failed");
            test_cancelled = true;
            return 0;
        }
        n -= m;
    }

    return 0;
}


void recv() {
    uint8_t buf[128];
    uint8_t expected[128];
    prng prandom(PRNG_INIT);
    int n = 0;

    Sleep(100);

    while (n < num_bytes && !test_cancelled) {
        DWORD k = 0;
        BOOL result = ReadFile(recv_hdl, buf, sizeof(buf), &k, NULL);
        if (!result) {
            perror("Read failed");
            test_cancelled = true;
            return;
        }
        if (k == 0) {
            fprintf(stderr, "No more data from %ls after %ld bytes\n", recv_port, n);
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
    send_hdl = open_com_port(send_port);
    if (send_hdl == INVALID_HANDLE_VALUE)
        return -1;

    if (wcscmp(send_port, recv_port) == 0) {
        recv_hdl = send_hdl;

    }
    else {
        recv_hdl = open_com_port(recv_port);
        if (recv_hdl == INVALID_HANDLE_VALUE) {
            CloseHandle(send_hdl);
            return -1;
        }
    }

    drain_port(recv_hdl);
    return 0;
}


void drain_port(HANDLE hdl) {
    uint8_t buf[16];
    DWORD k;
    BOOL result;
    do {
        result = ReadFile(recv_hdl, buf, sizeof(buf), &k, NULL);
    } while (result && k > 0);
}


void close_ports() {
    drain_port(recv_hdl);

    CloseHandle(send_hdl);
    if (send_hdl != recv_hdl)
        CloseHandle(recv_hdl);
}


HANDLE open_com_port(LPWSTR port)
{
    wchar_t port_filename[255];
    wcscpy_s(port_filename, L"\\\\.\\");
    wcscat_s(port_filename, port);

    HANDLE hComPort = CreateFile(port_filename,
        GENERIC_READ | GENERIC_WRITE, // read/write
        0,                            // no Sharing
        NULL,                         // no Security
        OPEN_EXISTING,// open existing port only
        0,            // non Overlapped I/O
        NULL);        // null for Comm Devices

    if (hComPort == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error in opening serial port\n");
        return hComPort;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    GetCommState(hComPort, &dcbSerialParams);

    dcbSerialParams.BaudRate = bit_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    BOOL result = SetCommState(hComPort, &dcbSerialParams);
    if (result == 0) {
        fprintf(stderr, "Fail to set baud rate (0x%08x)\n", GetLastError());
        CloseHandle(hComPort);
        return NULL;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    result = SetCommTimeouts(hComPort, &timeouts);
    if (result == 0) {
        fprintf(stderr, "Fail to set comm timeouts (0x%08x)\n", GetLastError());
        CloseHandle(hComPort);
        return NULL;
    }

    return hComPort;
}
