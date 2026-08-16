#pragma once

// OpenKNX Reg1 Fan-Addon-X2 board

// --- Reg1 Base pins (matches SEN-REG1-PowerMeter-3Phase) ---
#define PROG_LED_PIN 25
#define PROG_LED_PIN_ACTIVE_ON HIGH
#define PROG_BUTTON_PIN 23
#define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
#define SAVE_INTERRUPT_PIN 5
#define KNX_UART_NUM 0
#define KNX_UART_RX_PIN 1
#define KNX_UART_TX_PIN 0

// No dedicated status LED on Reg1 — reuse PROG_LED
#define STATUS_LED_PIN PROG_LED_PIN

// --- PWM polarity ---
// The level shifter drives an NMOS whose drain is pulled up to 5V, i.e. an inverting
// open-drain stage: GPIO high turns the FET on and pulls the fan input LOW.
// The firmware therefore inverts the duty cycle right before writing it to the pin.
//
// Note on power-up: with the GPIO still low the FET is off and the pull-up holds the fan
// input HIGH, which this fan family reads as full speed in direction B. What keeps that
// harmless is the load switch — it defaults to off (GPIO low = open) until the firmware
// enables it, so the fan is unpowered during that window.
#define FAN_PWM_ACTIVE_LOW 1

// --- Fan 1 (HW Channel A) ---
#define FAN1_S1_PWM_PIN 18   // PWM_A: via U3 level shifter -> Q1 open-drain
#define FAN1_SW_PIN     29   // POW_A: via U3 level shifter -> Q7/Q6 high-side switch
#define FAN1_TACHO_PIN  27   // TACHO_A: isolated via U2 optocoupler (active low)

// --- Fan 2 (HW Channel B) ---
#define FAN2_S1_PWM_PIN 28   // PWM_B: via U3 level shifter -> Q2 open-drain
#define FAN2_SW_PIN     26   // POW_B: via U3 level shifter -> Q9/Q8 high-side switch
#define FAN2_TACHO_PIN  17   // TACHO_B: isolated via U1 optocoupler (active low)
// #define PIN_SPARE       16   // Exposed on J1 pin 10, not connected on PCB

// --- No mirror outputs on this board ---
// One drive output per node; it carries speed AND direction (mid position = standstill).
// A Maico pair is wired with both fans on the same terminal — allowed because the fans'
// PWM inputs are high impedance. Both fans of one device always turn the same way anyway.
#define FAN1_S2_PWM_PIN -1
#define FAN2_S2_PWM_PIN -1
