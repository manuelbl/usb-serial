//
//  USB Serial
//
// Copyright (c) 2020 Manuel Bleichenbacher
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Pseudo random number generator
//

#include "prng.hpp"

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


int prng::verify(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (nbytes == 0) {
            bits = next();
            nbytes = 4;
        }
        if (buf[i] != (uint8_t)bits)
            return (int)i;
        bits >>= 8;
        nbytes--;
    }
    return -1;
}
