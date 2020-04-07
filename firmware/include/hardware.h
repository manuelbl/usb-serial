/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Hardware defintions
 */

#ifndef HARDWARE_H
#define HARDWARE_H

// --- USB pins, clocks and ISRs

#if defined(STM32F0)

#define USB_DP_PORT GPIOA
#define USB_DP_PIN GPIO12
#define USB_PORT_RCC RCC_GPIOA

#define USB_ISR usb_isr
#define USB_DRIVER st_usbfs_v2_usb_driver
#define USB_NVIC_IRQ NVIC_USB_IRQ

#elif defined(STM32F1)

#define USB_DP_PORT GPIOA
#define USB_DP_PIN GPIO12
#define USB_PORT_RCC RCC_GPIOA

#define USB_NVIC_IRQ NVIC_USB_LP_CAN_RX0_IRQ
#define USB_ISR usb_lp_can_rx0_isr
#define USB_DRIVER st_usbfs_v1_usb_driver

#else

#error "This code doesn't support this target!"

#endif

// --- USART pins, clocks and ISRs

#if defined(STM32F0)

#define USART USART2
#define USART_RX_DATA_REG USART2_RDR
#define USART_TX_DATA_REG USART2_TDR
#define USART_PORT GPIOA
#define USART_TX_GPIO GPIO2
#if defined(STM32F042F6)
#define USART_RX_GPIO GPIO3
#else
#define USART_RX_GPIO GPIO15
#endif
#define USART_RCC RCC_USART2

#elif defined(STM32F1)

#define USART USART2
#define USART_RX_DATA_REG USART2_DR
#define USART_TX_DATA_REG USART2_DR
#define USART_PORT GPIOA
#define USART_TX_GPIO GPIO2
#define USART_RX_GPIO GPIO3
#define USART_RCC RCC_USART2

#endif

// --- USART DMA channels and ISRs

#if defined(STM32F0)

#define USART_DMA DMA1
#define USART_DMA_TX_CHAN 4
#define USART_DMA_RX_CHAN 5
#define USART_DMA_NVIC_IRQ NVIC_DMA1_CHANNEL4_7_DMA2_CHANNEL3_5_IRQ
#define USART_DMA_RCC RCC_DMA

#elif defined(STM32F1)

#define USART_DMA DMA1
#define USART_DMA_TX_CHAN 7
#define USART_DMA_RX_CHAN 6
#define USART_DMA_TX_NVIC_IRQ NVIC_DMA1_CHANNEL7_IRQ
#define USART_DMA_RX_NVIC_IRQ NVIC_DMA1_CHANNEL6_IRQ
#define USART_DMA_RCC RCC_DMA1

#endif

// --- Additional RS-232 pins

#if defined(STM32F0)

#define DTR_PORT_RCC RCC_GPIOA
#define DTR_PORT GPIOA
#define DTR_PIN GPIO5

#define DSR_PORT_RCC RCC_GPIOB
#define DSR_PORT GPIOB
#define DSR_PIN GPIO1

#define DCD_PORT_RCC RCC_GPIOA
#define DCD_PORT GPIOA
#define DCD_PIN GPIO4

#define RTS_PORT_RCC RCC_GPIOA
#define RTS_PORT GPIOA
#define RTS_PIN GPIO1

#define CTS_PORT_RCC RCC_GPIOA
#define CTS_PORT GPIOA
#define CTS_PIN GPIO0

#elif defined(STM32F1)

#define DTR_PORT_RCC RCC_GPIOA
#define DTR_PORT GPIOA
#define DTR_PIN GPIO4

#define DSR_PORT_RCC RCC_GPIOA
#define DSR_PORT GPIOA
#define DSR_PIN GPIO5

#define DCD_PORT_RCC RCC_GPIOB
#define DCD_PORT GPIOB
#define DCD_PIN GPIO1

#define RTS_PORT_RCC RCC_GPIOA
#define RTS_PORT GPIOA
#define RTS_PIN GPIO1

#define CTS_PORT_RCC RCC_GPIOA
#define CTS_PORT GPIOA
#define CTS_PIN GPIO0

#endif

// --- LED pins and clocks

#if defined(STM32F0)

#if defined(STM32F042F6)

#define LED_POWER_PORT_RCC RCC_GPIOF
#define LED_POWER_PORT GPIOF
#define LED_POWER_PIN GPIO0

#else

#define LED_POWER_PORT_RCC RCC_GPIOB
#define LED_POWER_PORT GPIOB
#define LED_POWER_PIN GPIO3

#endif

#define LED_RX_PORT_RCC RCC_GPIOA
#define LED_RX_PORT GPIOA
#define LED_RX_PIN GPIO6

#define LED_TX_PORT_RCC RCC_GPIOA
#define LED_TX_PORT GPIOA
#define LED_TX_PIN GPIO7

#elif defined(STM32F1)

#define LED_POWER_PORT_RCC RCC_GPIOC
#define LED_POWER_PORT GPIOC
#define LED_POWER_PIN GPIO13

#define LED_RX_PORT_RCC RCC_GPIOA
#define LED_RX_PORT GPIOA
#define LED_RX_PIN GPIO6

#define LED_TX_PORT_RCC RCC_GPIOA
#define LED_TX_PORT GPIOA
#define LED_TX_PIN GPIO7

#endif

#endif
