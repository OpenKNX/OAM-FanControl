#pragma once

// MrSpieb HW-FanControl board
// https://github.com/mrspieb/HW-FanControl

#define PROG_LED_PIN_ACTIVE_ON 1
#define PROG_LED_PIN 25
#define PROG_BUTTON_PIN 4

#define STATUS_LED_PIN 24

#define KNX_UART_NUM 0
#define KNX_UART_TX_PIN 0
#define KNX_UART_RX_PIN 1

// PWM polarity: non-inverting. The board buffers 3.3V -> 5V with an SN74LVC125A, so the
// fan sees the same level the GPIO drives. FAN_PWM_ACTIVE_LOW stays undefined.

// Four PWM outputs, two per node. S1 and S2 of a node carry the IDENTICAL signal — one
// output per fan of a Maico pair, which always turn the same way. S2 is therefore a mirror
// of S1, not a second direction: the single output already carries speed AND direction
// (mid position = standstill).
#define FAN1_S1_PWM_PIN 9
#define FAN1_S2_PWM_PIN 8
#define FAN2_S1_PWM_PIN 7
#define FAN2_S2_PWM_PIN 6
#define FAN1_SW_PIN 13
#define FAN2_SW_PIN 12

// No tacho inputs on this board
#define FAN1_TACHO_PIN -1
#define FAN2_TACHO_PIN -1
