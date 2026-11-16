// ============================================================================
//  User settings - everything you are likely to want to change lives here.
// ============================================================================
#pragma once

// ---- Bambino Plus button output --------------------------------------------
// GPIO that "presses" the Bambino button (through an optocoupler / relay -
// never wire the ESP32 pin straight onto the Bambino's control board).
// 44 = RX pin of the UART connector. CHECK THIS against the schematic for
// your board revision before wiring; any free GPIO works.
#define BREW_PIN              44
// 1 = pin goes LOW to press (opto LED wired 3V3 -> resistor -> LED -> pin).
//     Safe choice: pins that idle/boot high cannot press the button by accident.
// 0 = pin goes HIGH to press (pin -> resistor -> LED -> GND).
#define BREW_PIN_ACTIVE_LOW   1

// ---- Shot sequence ---------------------------------------------------------
#define START_HOLD_MS         10000   // hold the button this long to start the shot
#define STOP_PRESS_MS         250     // press length that stops the shot
#define ABORT_GAP_MS          300     // pause between release and stop-press if STOP is hit during the hold
#define MAX_SHOT_MS           90000   // safety: stop the shot no matter what after this long
#define STOP_OFFSET_G         0.0f    // stop this many grams BEFORE target (to allow for drips). 0 = stop at target
#define START_MAX_WEIGHT_G    5.0f    // refuse to start if the scale reads more than this (i.e. it was not tared)

// ---- Scale -----------------------------------------------------------------
// 0 = user tares the scale by hand (default). 1 = display sends "tare + start timer".
#define TARE_ON_START         0
// The Ultra ignores timer commands in plain Weight mode. 1 = switch the scale
// to Flow mode on connect (uses a command observed from the Bookoo app, not in
// Bookoo's public protocol doc). 0 = user selects Flow mode on the scale.
#define FORCE_FLOW_MODE       0
// Only connect to a scale whose Bluetooth name contains this text, e.g. "246641".
// "" = first Bookoo scale found.
#define SCALE_NAME_FILTER     ""
#define SCALE_STALE_MS        1500    // no weight packets for this long = scale lost

// ---- Target weight / knob --------------------------------------------------
#define TARGET_DEFAULT_G      36.0f
#define TARGET_MIN_G          5.0f
#define TARGET_MAX_G          120.0f
#define TARGET_STEP_G         0.5f    // per knob click
#define ENC_EDGES_PER_CLICK   2       // half-cycle encoder on CrowPanel: 2 edges per physical detent
#define ENC_REVERSE           0       // 1 = swap knob direction

// ---- Board pins (CrowPanel 1.46" rotary, from Elecrow's wiki) ---------------
#define PIN_LCD_SCLK   10
#define PIN_LCD_MOSI   11
#define PIN_LCD_CS      9
#define PIN_LCD_DC      3
#define PIN_LCD_RST    14
#define PIN_LCD_BL     46
#define PIN_TOUCH_SDA   6
#define PIN_TOUCH_SCL   7
#define PIN_TOUCH_INT  13
#define PIN_TOUCH_RST   5
#define PIN_ENC_A      45
#define PIN_ENC_B      42
#define PIN_ENC_SW     41   // knob press button (active-low)
#define PIN_PWR_LED    40   // board power indicator (active-low)
#define PIN_PWR_1       1   // factory firmware drives these two high at boot
#define PIN_PWR_2       2
