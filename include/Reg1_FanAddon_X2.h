#pragma once

// OpenKNX REG1 Fan-Addon-X2, mounted on a REG1-Controller2040 V1 behind a REG1-Front-RGB.
//
// Controller and front come from OGM-HardwareConfig and are selected, not copied: KNX UART,
// save interrupt, prog button and the four WS2812B status LEDs on the front are defined there
// and stay correct when the hardware definition is corrected upstream. Only the addon's own
// pins live here — upstream does not know this board.
//
// What the two macros pull in:
//   OKNXHW_REG1_CONTROLLER2040_V1  KNX UART on GPIO 0/1, save interrupt, front connector pins
//   OKNXHW_REG1_FRONT_RGB          prog button on front pin 10 (GPIO 23), and the LED chain on
//                                  front pin 8 (GPIO 25) as OPENKNX_SERIALLED with 4 LEDs:
//                                  index 0 = prog (red), 1..3 = Info1..3 (green), plus the
//                                  colour calibration for these parts
//
// This replaces hand-written defines that had two of them wrong: GPIO 25 was declared as an
// ordinary prog LED, so the firmware drove a plain HIGH onto the WS2812 data line and all four
// LEDs came up white; and the save interrupt was on 5 instead of 3.
#define OKNXHW_REG1_CONTROLLER2040_V1
#define OKNXHW_REG1_FRONT_RGB

// --- Werksbelegung der drei Info-LEDs ---
// OGM-Common wertet diese optionalen Defines aus, solange die ETS-Checkbox "Standardbelegung"
// gesetzt ist - also ab Werk und bei jedem unkonfigurierten Geraet. Ohne sie zeigt Info1 den
// Geraetestatus und Info2/3 bleiben dunkel.
//
// Info1 behaelt bewusst den Geraetestatus: drei LEDs, zwei Luefter, da ist eine fuer den
// Geraetezustand gut angelegt. Die Luefter kommen auf Info2 und Info3.
//
// Die Werte sind Fan::LedFunctionBase + Kanalindex aus FanTypes.h und muessen zu den
// Enumerationswerten von PT-SLEDFunc in Fan.share.xml passen. Hier stehen sie als Zahl, weil
// Hardware-Header lange vor den Modulheadern eingebunden werden.
#define OPENKNX_INFOLED2_DEFAULT 110 // Luefter 1
#define OPENKNX_INFOLED3_DEFAULT 111 // Luefter 2

// --- PWM polarity ---
// The level shifter drives an NMOS whose drain is pulled up to 5V, i.e. an inverting
// open-drain stage: GPIO high turns the FET on and pulls the fan input LOW.
// The firmware therefore inverts the duty cycle right before writing it to the pin.
//
// Note on power-up: with the GPIO still low the FET is off and the pull-up holds the fan
// input HIGH, which this fan family reads as full speed in direction B. What keeps that
// harmless is the load switch — it defaults to off (GPIO low = open) until the firmware
// enables it, so the fan is unpowered during that window.
//
// Verified on hardware 2026-08-16: midpoint stands still, both directions run.
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

// --- How many fans this board can actually drive ---
// The ETS lets the user activate up to FAN_ChannelCount channels, which is a property of the
// application, not of the board. This table is the board's answer: everything beyond it has
// no pins and is reported as a configuration fault instead of silently doing nothing.
// Order matches the ETS channels.
#define FAN_BOARD_CHANNELS 2
#define FAN_BOARD_PIN_TABLE                                              \
    {                                                                    \
        {FAN1_S1_PWM_PIN, FAN1_S2_PWM_PIN, FAN1_SW_PIN, FAN1_TACHO_PIN}, \
        {FAN2_S1_PWM_PIN, FAN2_S2_PWM_PIN, FAN2_SW_PIN, FAN2_TACHO_PIN}, \
    }
