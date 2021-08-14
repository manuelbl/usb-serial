/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Common declarations
 */

#ifndef COMMON_H
#define COMMON_H

#include <libopencmsis/core_cm3.h>
#include <algorithm>

/**
 * @brief Initializes common services
 */
void common_init();

/**
 * @brief Gets the time.
 * 
 * @return number of milliseconds since a fixed time in the past
 */
uint32_t millis();

/**
 * @brief Delays execution (busy wait)
 * @param ms delay length, in milliseconds
 */
void delay(uint32_t ms);

/**
 * @brief Checks if the specified timeout has expired.
 * 
 * @param timeout timeout time, derived from a call to millis()
 * @return `true` if timeout time has been reached or passed, `false` otherwise
 */
bool has_expired(uint32_t timeout);

#endif
