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

## Requirements & Prerequisites

### Hardware
- **CrowPanel 1.46" Rotary Display** (ESP32-S3 round rotary knob display)
- **USB-C data cable** (ensure it supports data, not power-only)
- **Bookoo Themis Ultra** espresso scale
- **Bambino / Bambino Plus** (wired via optocoupler / relay to `BREW_PIN`)

### Software Installation

The recommended development and flashing environment is **VS Code + PlatformIO**:

1. **Install VS Code**: Download and install [Visual Studio Code](https://code.visualstudio.com/).
2. **Install PlatformIO IDE Extension**:
   - Open VS Code.
   - Go to the Extensions panel (`Ctrl+Shift+X` on Windows/Linux or `Cmd+Shift+X` on macOS).
   - Search for **PlatformIO IDE** and click **Install**.
   - Wait for PlatformIO to finish installing its core components and restart VS Code if prompted.
3. **Install USB / Serial Drivers** (if your PC does not recognize the ESP32-S3):
   - The CrowPanel uses the native USB-CDC or a CH340 / CP210x chip.
   - On Windows, check Device Manager under **Ports (COM & LPT)** when connected.

---

## How to Build & Flash (PlatformIO - Recommended)

1. **Open the project**:
   - In VS Code, choose **File > Open Folder...** and select `espresso_shot_stopper`.
2. **Connect the board**:
   - Plug the CrowPanel 1.46" rotary display into your computer using a USB-C data cable.
   - Make sure your board is detected on a COM port.
   - *Note on ESP32-S3 Bootloader:* If the board is brand new or not responding to flash commands, hold the **BOOT** button while plugging in or pressing **RST**, then release **BOOT** to enter flashing mode.
3. **Configure options (Optional)**:
   - Review and tweak settings in `src/config.h` (e.g. `BREW_PIN`, `STOP_OFFSET_G`, `SCALE_NAME_FILTER`).
4. **Build and Upload**:
   - **Using VS Code GUI**:
     - Click the **PlatformIO** alien icon on the left sidebar.
     - Under **Project Tasks > crowpanel146 > General**, click:
       - **Build** (check mark icon on bottom toolbar) to compile.
       - **Upload** (right arrow icon on bottom toolbar) to flash.
   - **Using PlatformIO CLI / Terminal**:
     ```powershell
     # Build the project
     pio run

     # Upload firmware to board
     pio run -t upload

     # (Optional) Specify COM port if not auto-detected:
     pio run -t upload --upload-port COMx
     ```
5. **Open Serial Monitor**:
   - In VS Code, click the plug icon on the bottom toolbar or run:
     ```powershell
     pio device monitor
     ```
   - Baud rate is preset to `115200`. You should see boot logs, Bluetooth scanning, and weight output.

---

## Alternative: Arduino IDE Setup

If you prefer using Arduino IDE (version 2.x recommended):

1. **Install ESP32 Board Support**:
   - In Arduino IDE, open **File > Preferences**.
   - Add the Espressif package URL to *Additional Boards Manager URLs*:
     `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Go to **Tools > Board > Boards Manager**, search for `esp32`, and install version **2.0.14** (recommended by Elecrow).
2. **Install Required Libraries**:
   Go to **Sketch > Include Library > Manage Libraries...** and install:
   - `LovyanGFX` @ **1.2.7** *(If `Panel_ST77961` fails to compile, use Elecrow's customized LovyanGFX from their CrowPanel demo repository)*
   - `lvgl` @ **8.3.11**
   - `NimBLE-Arduino` @ **1.4.3**
3. **Prepare the Sketch & LVGL Configuration**:
   - Put `bookoo.h`, `config.h`, `main.cpp`, and `shot.h` into an `espresso_shot_stopper` sketch folder.
   - Rename `main.cpp` to `espresso_shot_stopper.ino`.
   - Copy `include/lv_conf.h` into your Arduino libraries folder directly next to the `lvgl` directory (or inside your sketch root if configured).
4. **Configure Board Settings under `Tools`**:
   - **Board**: `ESP32S3 Dev Module`
   - **Flash Size**: `16MB (128Mb)`
   - **Partition Scheme**: `16M Flash (3MB APP/9.9MB FAT)` or `default 16MB`
   - **PSRAM**: `OPI PSRAM`
   - **Memory Type**: `qio_opi`
   - **USB Mode**: `Hardware CDC and JTAG`
   - **USB CDC On Boot**: `Enabled`
5. **Upload**: Select your board's COM port and click **Upload**.

---

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
