# Brew-by-weight knob

CrowPanel 1.46" rotary display + Bookoo Themis Ultra + Bambino Plus.

## What it does

1. Power on and tare the scale yourself. The display finds it over Bluetooth on its own.
2. Turn the ring to set the target weight (0.5 g per click, remembered across power cycles).
3. Tap **START**:
   - the Bambino button is pressed and held for 10 s, then released
   - the scale's timer is reset and started at the moment of the press
4. When the scale reads the target weight, the button is pressed for 250 ms and the scale's timer is stopped.

Tap **STOP** at any time to end the shot early. The shot is also stopped automatically
if the scale disconnects or after 90 s.

## Before first use

- **Put the scale in Flow mode.** The Ultra ignores timer commands in plain Weight mode.
  (Or set `FORCE_FLOW_MODE 1` in `src/config.h` to have the display switch it.)
- **Check `BREW_PIN` in `src/config.h`** against the board schematic, and wire it to the
  Bambino button through an optocoupler or small relay across the button contacts.
  Default wiring (`BREW_PIN_ACTIVE_LOW 1`): 3V3 -> 330 ohm -> opto LED -> `BREW_PIN`.
- Dry-run first: leave the Bambino unplugged from the opto, open the serial monitor,
  and watch the `button DOWN / up` lines while pouring water onto the scale.

## Build

PlatformIO: open this folder, then Upload. Library versions are pinned in `platformio.ini`.

Arduino IDE instead: put the four files from `src/` in a sketch folder (rename `main.cpp`
to `<folder>.ino`), copy `include/lv_conf.h` next to the `lvgl` library folder, install
LovyanGFX 1.2.7, lvgl 8.3.11, NimBLE-Arduino 1.4.3, ESP32 core 2.0.14, and select
ESP32S3 Dev Module / 16MB flash / OPI PSRAM / USB CDC On Boot: Enabled.

If the build can't find `Panel_ST77961`, use the LovyanGFX copy from Elecrow's repo
(`example/V1.0/Arduino/.../libraries`).

## Settings (`src/config.h`)

| Define | Default | Meaning |
|---|---|---|
| `TARE_ON_START` | 0 | 1 = display tares the scale when START is tapped |
| `START_HOLD_MS` | 10000 | length of the start press |
| `STOP_PRESS_MS` | 250 | length of the stop press |
| `STOP_OFFSET_G` | 0.0 | stop this many grams early to allow for drips |
| `START_MAX_WEIGHT_G` | 5.0 | START is refused above this ("Tare the scale first") |
| `MAX_SHOT_MS` | 90000 | hard time limit |
| `ENC_EDGES_PER_CLICK` | 4 | set to 2 if one step takes two clicks |
| `SCALE_NAME_FILTER` | "" | pin to one scale, e.g. "246641" |

## Test the logic on a PC

    g++ -std=c++17 -Isrc test/test_shot.cpp -o /tmp/t && /tmp/t
