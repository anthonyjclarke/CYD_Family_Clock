# Project: CYD Family Clock

World clock for ESP32-2432S028 (CYD) displaying 6 configurable timezones with dual display modes, optional environmental sensors, web configuration, OTA updates and a browser installer. Current version: 2.10.0 (`FIRMWARE_VERSION` in `include/config.h`).

## Hardware

- Board: ESP32-2432S028 "CYD" with ILI9341 2.8" TFT + XPT2046 touch
- Optional I2C sensors: BMP280, BME280, SHT3X, or HTU21D

## Non-Standard Pin Mapping

| Function | GPIO | Notes |
|----------|------|-------|
| Touch MISO | 39 | T_OUT — differs from common CYD docs |
| I2C SDA | 27 | Sensor interface |
| I2C SCL | 22 | Sensor interface |
| LDR | 34 | Analog input |

## Libraries

- TFT_eSPI ^2.5.43, WiFiManager ^2.0.16-rc.2, ArduinoJson ^7.0.4
- XPT2046_Touchscreen, Adafruit BMP280/BME280/SHT31/HTU21DF ^2.x
- Built-in: WiFi, ArduinoOTA, WebServer, Preferences, LittleFS
- Platform pinned to `espressif32@6.12.0` (arduino-esp32 2.0.17); unpinned resolves to 3.x
- Improv vendored in `lib/ImprovWiFi` (parser fix) – never add it to `lib_deps`

## NVS Storage

- **Namespace**: `worldclock`
- **Keys**: `homeLabel`, `homeTz`, `remote0Label`..`remote4Label`, `remote0Tz`..`remote4Tz`, `landscape`, `flip`, `fahrenheit`, `screenRot`, `flipInt`

## Sensor Selection

Uncomment ONE sensor type in `include/config.h`:
```cpp
#define USE_BMP280   // or USE_BME280, USE_SHT3X, USE_HTU21D
```

## Architecture Decisions

### Memory Leak Prevention — CRITICAL

Never use `setenv("TZ", ...)` in the main loop. ESP32's setenv leaks ~30-40 bytes per call. Solution: parse POSIX timezone strings once at config load into `ParsedTimezone` structs, then use `gmtime_r()` with manual offset calculation. Zero setenv calls = stable heap.

### Touch Detection Quirk

`XPT2046_Touchscreen::touched()` always returns true on this board variant. Use direct IRQ pin reading instead:
```cpp
bool isTouched() { return (digitalRead(36) == LOW); }
```

### Font Strategy

Hybrid system: bitmap fonts (setTextFont) for static elements (5-10× faster), smooth VLW fonts for time displays only. Batch draws by font type to minimize switches. The `.vlw` files load from LittleFS; if absent, `setFont()` falls back to bitmap fonts.

### Debug Macros

`main.cpp` defines its own `DBG_*` macros and every caller there passes its own `\n`. `include/debug.h` (for `src/network/`) appends the newline itself; `main.cpp` must not include it. `debugLevel` is non-static so both share it.

### Colon Blink

Redraw full time string ("HH:MM" or "HH MM") rather than erasing colon region — prevents alignment artifacts with proportional fonts.
## Web installer and releases

Release images come only from `.github/workflows/firmware.yml` on a `v*` tag on `main`. Details: `docs/WEB_INSTALLER.md`.

- Never put `firmware-merged.bin` in a manifest – it fills NVS with `0xFF`.
- `PROJECT_NAME` and `partitions_custom.csv` are frozen; a partition change needs an erase.
- LittleFS holds only the shipped fonts: Install and Update rewrite it. Settings go in NVS; the web UI stays in PROGMEM (`src/web_pages.h`).
- `improvTick()` must run at least every ~1 s.
- Before the next release, clear *Tests owed* in docs/WEB_INSTALLER.md (RUNBOOK 5b).
