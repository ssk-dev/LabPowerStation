# LAB POWER STATION

Portable Electronics Laboratory — Firmware for the **Waveshare ESP32-S3-Touch-LCD-4.3**

> **Phase 1 / Firmware `0.1.0-phase1`**
> This firmware boots the real display/touch/SD hardware of the Waveshare board and
> runs a complete, touch-operable LVGL user interface (Dashboard, Power Supply,
> Multimeter, Component Tester, Graph, Settings). **No external measurement or
> power-supply hardware (ADS1256, ACS758, real power stage) is connected yet.**
> All voltage/current/temperature values shown in the UI are produced by a clearly
> marked `SimulationSource` (`measurement.h`/`measurement.cpp`) and must **not** be
> mistaken for calibrated real-world readings.

---

## 1. Overview

LabPowerStation is the firmware groundwork for a future portable bench instrument
combining a 4‑channel lab power supply, a multimeter and a component tester behind
one 800×480 touchscreen. This repository currently implements:

- Real hardware bring-up (ESP32-S3, flash/PSRAM check, LCD, GT911 touch, SD card)
- A hardware-validation **boot screen checklist**
- A complete LVGL UI shell: status bar, bottom navigation, 6 screens
- A strictly layered architecture: `HARDWARE → MEASUREMENT → SYSTEM STATE → UI`
  (the UI never touches a GPIO, ADC or SPI bus directly)
- A `/settings.json` persistence layer on the SD/TF card
- A clean simulation boundary (`SIMULATION_MODE`) so the exact same UI code will
  later run unchanged against real ADCs/power electronics

Nothing in this firmware claims that external power or measurement hardware is
present — see [Section 10, Simulation Mode](#10-simulation-mode).

---

## 2. Hardware

| Item | Value | Source |
|---|---|---|
| Board | Waveshare **ESP32-S3-Touch-LCD-4.3** | Official Waveshare product/documentation |
| MCU | ESP32-S3, 240 MHz | Waveshare docs |
| Flash | 16 MB | Waveshare docs |
| PSRAM | 8 MB, **OPI** mode | Waveshare docs |
| Display | 4.3" IPS, 800×480, RGB interface | Waveshare docs |
| LCD controller | ST7262 | Waveshare docs |
| Touch controller | GT911 (capacitive, I²C) | Waveshare docs |
| IO Expander | CH422G (backlight, LCD/touch reset, SD chip-select, USB-select) | Waveshare docs / `BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3.h` |
| SD/TF card | SPI, CS routed through CH422G **EXIO4** | Waveshare `03_SD_Test` example |

### Pins actually used (confirmed from the official Waveshare documentation —
### never invented; see `prompt-software.txt` constraint)

| Function | Pin |
|---|---|
| Touch SDA | GPIO8 |
| Touch SCL | GPIO9 |
| Touch IRQ | GPIO4 |
| SD MOSI | GPIO11 |
| SD CLK | GPIO12 |
| SD MISO | GPIO13 |
| SD CS | **EXIO4** on the CH422G IO expander (not a raw ESP32 GPIO) |
| LCD data/control | GPIO0, 1, 2, 3, 5, 7, 10, 14, 17, 18, 21, 38, 39, 40, 41, 42, 45, 46, 47, 48 (driven by the ESP32_Display_Panel board driver, not by application code) |

The LCD/touch pin list and the CH422G EXIO numbering (TP_RST=1, LCD_BL=2,
LCD_RST=3, SD_CS=4, USB_SEL=5) come from the board driver's own
`BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3.h` definition inside the
**ESP32_Display_Panel** library — application code never re-defines or guesses a pin.

**No hardware beyond the Waveshare board itself (display, touch, SD slot) is
required to run this firmware.**

---

## 3. Software / Libraries

| Component | Library | Version used |
|---|---|---|
| Board driver (LCD, touch, IO-expander glue) | **ESP32_Display_Panel** | 1.0.4 |
| IO expander driver (CH422G) | **ESP32_IO_Expander** | 1.1.1 |
| Internal helper used by the board driver | **esp-lib-utils** | 0.2.3 |
| GUI | **LVGL** | **8.4.0** (pinned — see `lv_conf.h`) |
| SD card / filesystem | `SD`, `FS`, `SPI` | bundled with the ESP32 Arduino core |
| Settings JSON (de)serialization | `cJSON` | bundled with the ESP32 Arduino core (no extra library needed) |

> `ArduinoJson` is present in the shared libraries folder but is **not** used by
> this firmware; settings persistence uses the `cJSON` implementation that ships
> with the ESP32 Arduino core.

All libraries are installed in a shared folder **outside** the default Arduino
`libraries` directory:

```
C:\Workspace\tools\projekte\libraries
```

If you compile with the Arduino IDE, add this path under
**File → Preferences → Sketchbook location** is *not* sufficient for extra
library search paths; instead add it as an additional "Custom Libraries" path,
or use `arduino-cli` as shown in [Section 7](#7-building-with-arduino-cli).

---

## 4. Arduino IDE Settings

Install the **ESP32 Arduino Core** (via Boards Manager) and select:

| Setting | Value |
|---|---|
| Board | `ESP32S3 Dev Module` |
| USB CDC On Boot | Enabled (recommended for Serial Monitor over USB) |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | Any scheme that fits a 16 MB flash with OTA/room for your sketch (default 16MB scheme works) |
| PSRAM | **OPI PSRAM** |
| CPU Frequency | **240MHz (WiFi)** |
| Upload Speed | 921600 (or lower if you see upload errors) |

These values match `arduino-settings.png` and the requirements in
`prompt-software.txt` (Section 39, "ARDUINO BOARD SETTINGS").

---

## 5. Required Libraries (Library Manager names)

Install these via **Arduino IDE → Library Manager**, or point the build at the
pre-installed copies in `C:\Workspace\tools\projekte\libraries`:

- `ESP32_Display_Panel`
- `ESP32_IO_Expander`
- `esp-lib-utils`
- `lvgl` (**must stay at 8.4.0** — `lv_conf.h` in this project is written for the
  LVGL 8.x API and `display.cpp` has compile-time guards against other major
  versions)

`SD`, `FS`, `SPI`, `cJSON` and `Arduino.h` come from the ESP32 core itself — no
separate installation needed.

---

## 6. Opening the Project

1. Open the Arduino IDE.
2. **File → Open...** → select `LabPowerStation/LabPowerStation.ino`.
3. Apply the board settings from [Section 4](#4-arduino-ide-settings).
4. Make sure the libraries from [Section 5](#5-required-libraries-library-manager-names)
   are visible to the IDE (either installed normally, or via a custom library path
   if your IDE version supports it).

---

## 7. Building with `arduino-cli`

This project was built and verified using `arduino-cli` with an **external**
library folder, so the Arduino IDE's own library directory does not need to
contain anything:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile `
    --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,CPUFreq=240 `
    --libraries 'C:\Workspace\tools\projekte\libraries' `
    'C:\Workspace\tools\projekte\Labornetzteil\LabPowerStation'
```

The explicit board options (`CDCOnBoot=cdc`, `FlashSize=16M`, `PSRAM=opi`,
`CPUFreq=240`) are required to match the settings in [Section 4](#4-arduino-ide-settings) —
the bare `esp32:esp32:esp32s3` FQBN silently falls back to the ESP32 core's
defaults (4 MB flash, PSRAM disabled, USB CDC on boot disabled), which does not
match this project's hardware or the "no automatic reset after flashing"
behaviour documented in [Section 8](#8-debugging--serial-monitor).

Verified result (16 MB flash partition, real hardware flash via `COM10`):
**823272 bytes (26%) flash**, **29896 bytes (9%) RAM**, 0 errors.

### Flashing

Identify the board's COM port (Windows Device Manager or):

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' board list
```

Then upload:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' upload `
    --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,CPUFreq=240 `
    --port COM10 `
    'C:\Workspace\tools\projekte\Labornetzteil\LabPowerStation'
```

(Replace `COM10` with your actual port if different.)

With `CDCOnBoot=cdc` set, `esptool` performs a working automatic
**"Hard resetting via RTS pin..."** at the end of a successful upload, and the
native USB-CDC serial port (`COM10`) stays available immediately after reset —
no manual RST/BOOT button press is required. If `CDCOnBoot` is left at its
default (disabled), the native USB port only appears once the sketch itself
calls `Serial.begin()`, which looks like "no automatic reset happened" even
though the reset pulse was sent correctly.

---

## 8. Debugging / Serial Monitor

Open the Serial Monitor at **115200 baud**. On boot you should see:

```
================================
LAB POWER STATION
================================
LCD: 800x480
Backlight: ON
GT911 driver: initialized
LVGL: 8.4.0
Waiting for visual/touch confirmation. External measurement hardware: NOT IMPLEMENTED.
```

If the SD card cannot be mounted, a non-fatal line is printed:

```
StorageManager: no SD/TF card found, settings.json persistence disabled.
```

Debug logging is controlled by a single compile-time switch in `config.h`:

```cpp
#define DEBUG_ENABLED true
```

Set it to `false` to silence all `Serial.print*` diagnostics.

---

## 9. Boot Screen / Hardware Test

Before the Dashboard appears, the firmware runs a mandatory hardware validation
screen (`UI::hardwareTest()` in `ui.cpp`), checking:

```
ESP32-S3                       OK
Flash                       16 MB
PSRAM                         8 MB
LCD                    800 x 480 / initialisiert
GT911                         Treiber initialisiert
LVGL                       8.4.0
SD-Karte                       13.4GB / 15.0GB   (or: nicht erkannt)
```

The SD line shows live card capacity (used/total, decimal GB, matching the
convention printed on card packaging) when a card is mounted, or
`nicht erkannt` ("not detected") when none is present — the firmware continues
to boot normally either way, since the card is optional.

Touch the screen to continue into the Dashboard. The validation sequence covers
items 1–8 from the original specification (ESP32-S3 boot, PSRAM, LCD init,
800×480, backlight, GT911 init, LVGL init, touch confirmation).

---

## 10. Simulation Mode

```cpp
// config.h
#define SIMULATION_MODE true
```

With `SIMULATION_MODE true` (the current and only tested state):

- No ADS1256/ACS758 or any other external ADC/current-sense hardware is required.
- All `PowerChannel` and `MeasurementData` values are generated by
  `SimulationSource` (`measurement.cpp`), which models simple resistive loads and
  time-varying values — clearly commented as **DEMO ONLY** in the source.
- The UI is fully operable: every screen, slider, numeric keypad and navigation
  path works end-to-end against this simulated data.
- `PowerSupplyManager` (`power_supply.h`/`.cpp`) only ever updates `SystemState`;
  it never writes a GPIO and explicitly documents that it "never writes GPIOs or
  claims hardware protection."

When real hardware is eventually attached, `SIMULATION_MODE` is intended to be
set to `false` and the simulation source swapped for real ADC/ACS758-backed
measurement and power-control classes — **without changing a single line of UI
code**, because the UI only ever reads `SystemState` / `PowerChannel` /
`MeasurementData`, never a sensor or GPIO directly.

> The firmware does **not** claim OVP/OCP/OTP protection is guaranteed by software.
> Any future real build **must** implement independent hardware protection; the
> UI only visualizes/acknowledges fault state (`Fault` enum in `system_state.h`).

---

## 11. LVGL Configuration

- LVGL version is pinned to **8.4.0**. `display.cpp` contains `#error` guards that
  stop compilation if a different major/minor LVGL version is detected, because
  `lv_conf.h` (color depth, custom allocator hooks) is written specifically for
  the 8.x API.
- `LV_COLOR_DEPTH` is `16`, `LV_COLOR_16_SWAP` is `0` — matching the RGB565 panel
  configuration below.
- `esp_panel_board_custom_conf.h` overrides the board's default LCD color format
  from RGB888 to **RGB565**, following the Waveshare
  `Arduino/examples/09_lvgl_Porting` reference sketch.
- Rendering uses a single-task RGB565 flush (`DisplayManager::flush`) — LVGL is
  only ever touched from the Arduino `loop()` task, never from an ISR or a second
  FreeRTOS task, avoiding the need for a mutex around LVGL calls.

---

## 12. Touch Test

Touch input is handled by `TouchManager` (`touch.h`/`.cpp`), wrapping the GT911
driver exposed by the board library. The boot screen requires at least one touch
event before the Dashboard loads, which doubles as a basic functional touch test.
A dedicated, more detailed test overlay (color/touch coordinates/button
counter/live memory) is available via **Settings → SYSTEM → long press**
(`UI::testMode()`), as required for the project's test-mode feature.

---

## 13. SD Card / `settings.json`

Real SD card support (`storage.h` / `storage.cpp`, class `StorageManager`):

- **Mounting**: SPI pins MOSI=GPIO11, CLK=GPIO12, MISO=GPIO13; chip-select is
  **not** a raw GPIO — it is driven through the board's already-initialized
  CH422G IO expander (`EXIO4`), reusing the same expander instance the board
  driver uses for backlight/touch-reset (no duplicate I²C init).
- **No card present** is treated as a **non-fatal** condition: the firmware
  boots normally, the boot screen shows `nicht erkannt`, the status bar shows
  `--`, and all settings simply stay in RAM for that session.
- **Settings persistence**: user-changeable settings are grouped by category and
  written to `/settings.json` on the card whenever a setting changes
  (`state.settingsDirty`), e.g.:

  ```json
  {
      "display": { "contrast": 100, "night": false },
      "measurement": { "updateMs": 100 },
      "fan": { "fanAuto": true, "fanManual": 35 },
      "logging": { "logging": false }
  }
  ```

  If `/settings.json` does not exist yet, the firmware writes the current
  in-memory defaults on first boot. If the file exists but is not valid JSON,
  the firmware logs a warning and keeps the RAM defaults rather than crashing.
- **Capacity reporting**: `SD.totalBytes()` / `SD.usedBytes()` are converted to
  decimal GB (÷ 1,000,000,000) and shown both on the boot screen checklist and in
  the top status bar (e.g. `13.4GB / 15.0GB`), refreshed after mount and after
  every settings save.
- **Logging/CSV export** is unrelated to `/settings.json`: up to 120 recent
  samples are kept in a RAM ring buffer and can be exported as CSV over the
  Serial connection (115200 baud) from **Settings → LOGGING → EXPORT CSV** — this
  does **not** write a file to the SD card.

---

## 14. Project Structure

```
LabPowerStation/
├── LabPowerStation.ino        Setup/loop, task scheduling, startup sequence
├── config.h                   SIMULATION_MODE/DEBUG_ENABLED, UI/demo constants
├── system_state.h / .cpp      Shared data model (PowerChannel, MeasurementData,
│                              MeterState, Settings, SystemState) — the only
│                              thing the UI is allowed to read
├── hardware.h / .cpp          Board bring-up (ESP32-S3/flash/PSRAM validation),
│                              FanManager (simulated %), NetworkManager (offline)
├── display.h / .cpp           LVGL RGB565 display port (single task)
├── touch.h / .cpp             GT911 touch input adapter
├── storage.h / .cpp           SD card mount, /settings.json persistence,
│                              RAM ring-buffer + Serial CSV export
├── measurement.h / .cpp       MeasurementManager + SimulationSource (demo data),
│                              30-minute bounded history buffer
├── power_supply.h / .cpp      PowerSupplyManager — simulation command boundary
├── ui.h / .cpp                Shared UI building blocks, status bar, bottom nav,
│                              boot screen / hardware test, screen switching
├── ui_dashboard.h / .cpp      4-channel overview + input/output power/efficiency
├── ui_power_supply.h / .cpp   Fixed outputs (3.3V/5V/12V) + Variable output screen
├── ui_multimeter.h / .cpp     DCV/ACV/DCA/ACA/Ω/Continuity/Diode/Capacitance/Hz
├── ui_component_tester.h/.cpp Component tester UI (ZIF socket, auto test, result)
├── ui_graph.h / .cpp          Live chart: voltage/current/power, LIVE/1/5/30 min
├── ui_settings.h / .cpp       10-category settings menu + detail/edit screens
├── esp_panel_board_custom_conf.h  Board driver override (RGB565 color mode)
└── lv_conf.h                  LVGL 8.4.0 configuration (RGB565, custom allocator)
```

Data flow is strictly one-directional:

```
HARDWARE  →  MEASUREMENT  →  SYSTEM STATE  →  UI
```

The UI never calls `analogRead()`, touches SPI/I²C, or performs sensor math —
it only reads `SystemState` fields such as
`context().power.getChannel(POWER_VARIABLE).voltage`.

---

## 15. Current Limitations / Future Hardware Integration

This is explicitly a **Phase 1** firmware. The following are *not yet*
implemented and the code never claims otherwise:

- No ADS1256 ADC driver (voltage/current channels for 3.3V/5V/12V/VAR)
- No ACS758 current-sense driver
- No real variable power-supply control stage (no DAC/PWM output drives a
  physical supply)
- No WiFi (`NetworkManager::update()` always reports `wifiConnected = false`)
- No PWM fan control (fan percentage shown in the UI is a simulated demo value)
- No touch calibration routine (GT911 is used with factory/manufacturer
  coordinates, as it does not require calibration per the Waveshare reference)

The intended future hardware tree (from the original specification) is:

```
ESP32-S3
   ├── LVGL Display            (implemented — real hardware)
   ├── ADS1256                 (not implemented)
   │      ├── 3.3V/5V/12V/VAR current
   │      └── 3.3V/5V/12V/VAR voltage
   ├── ACS758                  (not implemented)
   ├── variable power supply   (not implemented)
   ├── fan control             (not implemented, demo % only)
   ├── storage                 (implemented — real SD card + settings.json)
   └── future multimeter hardware (not implemented)
```

Because the UI only talks to `SystemState`/`PowerChannel`/`MeasurementData`, this
hardware can be added later by implementing new `MeasurementSource`,
`PowerSupplyManager` and `FanManager` backends, without rewriting the LVGL UI.

---

## 16. Arduino IDE Flash Checklist

1. Install the **Arduino IDE** (2.x recommended).
2. **File → Preferences → Additional Boards Manager URLs** → add the ESP32
   boards URL, then install the **esp32** board package via Boards Manager.
3. **Tools → Board** → select **ESP32S3 Dev Module**.
4. **Tools → Flash Size** → **16MB (128MB)**.
5. **Tools → PSRAM** → **OPI PSRAM**.
6. **Tools → CPU Frequency** → **240MHz (WiFi)**.
7. **Tools → USB CDC On Boot** → **Enabled** (without this, the native USB
   serial port only appears once the sketch calls `Serial.begin()`, which looks
   like the automatic reset after flashing is not working).
8. Install/point to the required libraries (Section 5) — `ESP32_Display_Panel`,
   `ESP32_IO_Expander`, `esp-lib-utils`, `lvgl` 8.4.0.
9. Open `LabPowerStation.ino`.
10. **Verify/Compile** (or the `arduino-cli compile` command from Section 7).
11. Select the correct **Port** (e.g. `COM10`) under **Tools → Port**.
12. **Upload** (or the `arduino-cli upload` command from Section 7).
13. Open **Tools → Serial Monitor** at **115200 baud**.
14. Confirm the boot banner and hardware checklist appear, then touch the screen
    to continue to the Dashboard.

---

*This README documents only hardware and APIs confirmed against the official
Waveshare ESP32-S3-Touch-LCD-4.3 documentation and the board driver's own
definitions — no pins, library names or APIs were invented.*
