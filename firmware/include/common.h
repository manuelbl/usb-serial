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

#define IRQ_PRI_UART (2 << 6)
#define IRQ_PRI_UART_DMA (2 << 6)

// timer and USB must be at the same priority level
// so they don't mutually interrupt themselves
#define IRQ_PRI_UART_TIM (2 << 6)
#define IRQ_PRI_USB (2 << 6)

void common_init();

// Returns the number of milliseconds
// since a fixed time in the past
uint32_t millis();

// Delays the specified number of milliseconds
// (busy wait)
void delay(uint32_t ms);

// Checks if the specified timeout time has been reached or passed
bool has_expired(uint32_t timeout);


class irq_guard
{
public:
    irq_guard()
    {
        __disable_irq();
    }

    ~irq_guard()
    {
        __enable_irq();
    }
};

#endif
