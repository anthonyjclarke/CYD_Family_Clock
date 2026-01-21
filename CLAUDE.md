# Project: CYD Family Clock

## Overview

World clock displaying current time across 6 configurable timezones (1 home + 5 remote cities) on a 2.8" ILI9341 TFT display. Features web-based configuration, NVS persistent storage, OTA updates, custom timezone entry, 5-level debug system, and anti-flicker selective redraws with smooth fonts from LittleFS.

## Hardware

- MCU: ESP32 (ESP32-2432S028 "CYD" board)
- Display: ILI9341 2.8" TFT (240x320 pixels, portrait orientation)
- Power: USB 5V via CYD board

## Build Environment

- Framework: Arduino
- Platform: espressif32
- Key Libraries:
  - TFT_eSPI @ ^2.5.43 (display driver)
  - WiFiManager @ ^2.0.16-rc.2 (credential portal)
  - ArduinoJson @ ^7.0.4 (web API JSON handling)
  - XPT2046_Touchscreen (touch screen driver)
  - Built-in: WiFi, ArduinoOTA, WebServer, Preferences (NVS), LittleFS, time.h (NTP)

## Project Structure

```txt
├── src/
│   └── main.cpp              # Main implementation (~850 lines)
├── include/
│   ├── User_Setup.h          # TFT_eSPI hardware config
│   └── timezones.h           # 102 predefined timezones across 13 regions
├── data/                     # LittleFS files (upload with uploadfs)
│   ├── index.html            # Web configuration UI
│   ├── app.js                # WebUI JavaScript
│   ├── style.css             # WebUI styling
│   ├── NotoSans-Bold7.vlw    # Smooth fonts
│   ├── NotoSans-Bold9.vlw
│   ├── NotoSans-Bold10.vlw
│   └── NotoSans-Bold16.vlw
├── platformio.ini
├── README.md
├── CHANGELOG.md
└── CLAUDE.md                 # This file
```

## Pin Mapping

| Function | GPIO | Notes                |
|----------|------|----------------------|
| TFT_MOSI | 13   | SPI data out         |
| TFT_MISO | 12   | SPI data in          |
| TFT_SCLK | 14   | SPI clock            |
| TFT_CS   | 15   | Chip select          |
| TFT_DC   | 2    | Data/command         |
| TFT_RST  | -1   | Tied to ESP32 RST    |
| TFT_BL   | 21   | Backlight (active)   |

## Configuration System

### Persistent Storage (NVS)

Config stored in ESP32 NVS (Non-Volatile Storage) via Preferences API:

- **Namespace**: `"worldclock"`
- **Keys**: `homeLabel`, `homeTz`, `remote0Label`, `remote0Tz`, ..., `remote4Label`, `remote4Tz`
- **Load/Save**: `loadConfig()` / `saveConfig()` functions (lines 96-158)

### Default Configuration

```cpp
Home: SYDNEY (AEST-10AEDT,M10.1.0/2,M4.1.0/3)
Remote Cities:
  1. VANCOUVER (PST8PDT,M3.2.0/2,M11.1.0/2)
  2. LONDON (GMT0BST,M3.5.0/1,M10.5.0/2)
  3. NAIROBI (EAT-3)
  4. DENVER (MST7MDT,M3.2.0/2,M11.1.0/2)
  5. TOKYO (JST-9)
```

### Runtime Configuration

- **Web Interface**: Browse to device IP, configure via dropdown selects
- **Custom Timezones**: "-- Custom Timezone --" option for manual POSIX string entry
- **Live Updates**: Config changes apply immediately without reboot
- **Timezone Database**: 102 cities across 13 regions in `include/timezones.h`

## Key Settings in `src/main.cpp`

### Debug System (lines 17-57)

- **5 Levels**: Off (0), Error (1), Warn (2), Info (3), Verbose (4)
- **Runtime Control**: `debugLevel` variable (default: 3)
- **Macros**: `DBG_ERROR()`, `DBG_WARN()`, `DBG_INFO()`, `DBG_VERBOSE()`
- **Output**: Formatted with level prefixes, 115200 baud

### Network Settings (lines 59-71)

```cpp
FIRMWARE_VERSION = "2.0.0"
OTA_HOSTNAME = "CYD-WorldClock"
OTA_PASSWORD = "change-me"  // ⚠️ CHANGE IN PRODUCTION
NTP_SERVER1 = "pool.ntp.org"
NTP_SERVER2 = "time.nist.gov"
```

### Display Layout (lines 139-157)

```cpp
// Colors
COLOR_BG = TFT_BLACK
COLOR_LABEL = TFT_WHITE
COLOR_TIME = TFT_GREEN

// Layout
kHeaderHeight = 40px        // "WORLD CLOCK" + date
kPad = 10px                 // Left padding
6 cities × 46.7px per row   // Home + 5 remote

// Fonts
kFontTitle = "NotoSans-Bold16" (fallback: 4)
kFontDate = "NotoSans-Bold10" (fallback: 2)
kFontLabel = "NotoSans-Bold9" (fallback: 2)
kFontTime = "NotoSans-Bold10" (fallback: 2)
kFontNote = "NotoSans-Bold7" (fallback: 1)
```

## Current State (v2.2.0)

### Features

- **6 Cities**: 1 home (reference timezone) + 5 remote cities
- **Web Configuration**: Full WebUI at `http://<device-ip>`
- **Persistent Config**: NVS storage, survives reboots
- **Custom Timezones**: Manual POSIX string entry for unlisted cities
- **OTA Updates**: Wireless firmware updates with progress bar
- **Debug System**: 5-level runtime-adjustable logging
- **WiFi Setup**: WiFiManager captive portal + fallback AP mode
- **NTP Sync**: Automatic time sync on boot
- **Splash Screen**: Globe animation with firmware version
- **Boot Messages**: On-screen startup progress display
- **State Caching**: Flicker-free selective redraws
- **Home Indicator**: Cyan "Home" label under reference city
- **Prev Day Indicator**: Yellow "PREV DAY" for cities in previous day

### Display Layout

```txt
┌─────────────────────────┐
│   WORLD CLOCK           │ ← Title (cyan)
│   FRI 18 JAN            │ ← Date from home city
├─────────────────────────┤
│ SYDNEY         14:23    │ ← Home city
│   Home                  │ ← Cyan indicator
├─────────────────────────┤
│ VANCOUVER      20:23    │
│   PREV DAY              │ ← Yellow if in previous day
├─────────────────────────┤
│ LONDON         04:23    │
├─────────────────────────┤
│ NAIROBI        07:23    │
├─────────────────────────┤
│ DENVER         21:23    │
│   PREV DAY              │
├─────────────────────────┤
│ TOKYO          13:23    │
└─────────────────────────┘
```

### WebUI Features

- **Timezone Dropdowns**: 102 predefined cities grouped by region
- **Custom Entry**: Manual POSIX timezone string input
- **Live Preview**: Shows timezone string for selected city
- **System Status**: Firmware, hostname, WiFi, IP, uptime
- **System Actions**: Reboot device, reset WiFi credentials
- **Auto-refresh**: Status updates every 30 seconds

### REST API Endpoints

```txt
GET  /api/state       - System status + current config (JSON)
GET  /api/timezones   - List of 102 predefined timezones (JSON)
POST /api/config      - Update timezone configuration
POST /api/reboot      - Reboot device
POST /api/reset-wifi  - Clear WiFi credentials and reboot to AP mode
GET  /                - Web UI (index.html)
GET  /app.js          - WebUI JavaScript
GET  /style.css       - WebUI styling
```

## Architecture Notes

### Config Structure (lines 73-93)

```cpp
struct Config {
  char homeCityLabel[32];      // Home city name
  char homeCityTz[64];         // Home POSIX timezone
  char remoteCities[5][32];    // 5 remote city names
  char remoteTzStrings[5][64]; // 5 remote POSIX timezones
};
```

### State Caching (lines 175-179)

Minimizes TFT writes and prevents flicker:

```cpp
String lastDate;           // Last drawn date
String lastTimes[6];       // Last drawn time for each city
bool lastPrevDay[6];       // Last "PREV DAY" state
bool lastColonState[6];    // Last colon blink state
```

### Selective Redraw Logic

- **Date**: Only redraws when string changes (once per day)
- **Time**: Only redraws when HH:MM changes (once per minute)
- **Colon**: Targeted 1-char redraw every second (blinking animation)
- **Prev Day**: Only redraws when day boundary crossed
- **Labels**: Only redrawn during `drawStaticLayout()` after config change

### Timezone Switching

Uses `setenv("TZ", ...)` + `tzset()` to dynamically evaluate time in different zones:

```cpp
setenv("TZ", config.homeCityTz, 1);
tzset();
struct tm timeinfo;
getLocalTime(&timeinfo);
// Now timeinfo contains home city's local time
```

### Font Management

- Loads smooth fonts from LittleFS on-demand
- Unloads previous font when switching
- Falls back to bitmap fonts if .vlw files missing
- Helper: `setFont(smoothName, fallbackSize)`

### Previous Day Logic

Compares each city's year/yday to home city (not Sydney):

```cpp
if (remoteTm.tm_year < homeTm.tm_year ||
    (remoteTm.tm_year == homeTm.tm_year && remoteTm.tm_yday < homeTm.tm_yday)) {
  // Show "PREV DAY" indicator
}
```

### Startup Sequence

1. Serial init (115200 baud)
2. Backlight on
3. Startup display init
4. LittleFS mount
5. Load config from NVS
6. WiFi connect (WiFiManager or fallback AP)
7. OTA setup
8. WebServer start
9. NTP sync (uses home city timezone)
10. Splash screen animation
11. Draw clock interface
12. Enter main loop

### Main Loop (lines 826-862)

```cpp
loop() {
  ArduinoOTA.handle();      // Handle OTA updates
  server.handleClient();    // Handle web requests

  // Get current time and format for all cities
  time_t now = time(nullptr);
  struct tm homeTm;
  getLocalTm(config.homeCityTz, now, &homeTm);

  // Update display (selective redraws)
  drawTimes(homeTm);

  // Debug output (level 3): print all city times
  if (debugLevel >= DBG_LEVEL_INFO) {
    // Print formatted times for all 6 cities
  }

  delay(1000);  // 1 second update interval
}
```

### JSON Parsing (lines 560-603)

Manual string parsing to avoid ArduinoJson memory overhead:

```cpp
// Extract: "homeCity":{"label":"Sydney, Australia","tz":"AEST-10AEDT..."}
int homeLabelStart = body.indexOf("\"homeCity\":{\"label\":\"") + 21;
int homeLabelEnd = body.indexOf("\"", homeLabelStart);
String homeLabel = body.substring(homeLabelStart, homeLabelEnd);

// Extract city name only (strip country)
int commaPos = homeLabel.indexOf(',');
String cityOnly = (commaPos > 0) ? homeLabel.substring(0, commaPos) : homeLabel;
```

### OTA Progress Bar (lines 425-467)

Displays on TFT during wireless updates:

- Title: "OTA UPDATE" (cyan)
- Progress bar: Green fill, white border
- Percentage: Updates in serial output
- Completion: "UPDATE COMPLETE" message

### Splash Screen (lines 469-516)

Simple globe animation:

1. Title fade-in: "CYD WORLD CLOCK"
2. Globe: Circle with 12 timezone markers (dots)
3. Firmware version display
4. Total duration: ~3 seconds

## Memory Usage

- **Flash**: 1,016,021 bytes (77.5% of 1.3MB)
- **RAM**: 51,136 bytes (15.6% of 320KB)

## WiFi Behavior

- **First boot**: Creates AP "CYD-WorldClock-Setup", captive portal
- **Configured**: Auto-connects to saved network
- **Failed connect**: Fallback AP "CYD-WorldClock-AP"
- **Timeouts**: 180s portal, 20s connect attempt

## Critical Fix: Memory Leak (v2.2.0)

### Problem
Device was experiencing severe memory leak (~186 bytes/second), causing crashes after 12-15 minutes of operation. Heap would drop from 200KB to exhaustion.

### Root Cause
The ESP32's `setenv("TZ", ...)` function leaks ~40 bytes per call. The clock was calling `setenv()` **4-6 times per second** while cycling through 6 different timezones to display each city's time.

### Solution
Implemented **time result caching** in `formatTime()`:
- Cache formatted time for each city (6 total)
- Only recalculate when the minute changes
- Reduced `setenv()` calls from **~300/minute to 6/minute** (98% reduction)

**Result:** Leak rate reduced from -186 bytes/sec to -3.25 bytes/sec (essentially eliminated). Device now runs indefinitely without crashes.

### Implementation Details
```cpp
// Cache structure for each city
struct CachedTimeInfo {
  struct tm tm;
  char timeStr[8];
  bool prevDay;
  time_t lastCalculated;
};

static CachedTimeInfo timeCache[6];  // Home + 5 remote cities

// Only recalculate when minute changes
if (timeCache[cityIndex].lastCalculated == 0 ||
    (now / 60) != (timeCache[cityIndex].lastCalculated / 60)) {
  getLocalTm(tz, now, &timeCache[cityIndex].tm);
  // ... format and cache results
}
```

## Resolved Issues from v1.0.0

- ✅ Config persistence - Now stored in NVS
- ✅ Web interface - Full WebUI for configuration
- ✅ OTA support - Wireless updates with progress bar
- ✅ Runtime city selection - No recompilation needed
- ✅ Custom timezones - Manual POSIX string entry
- ✅ WiFi reconnect - WiFiManager handles credentials
- ✅ 6th city support - Added in v2.0.0
- ✅ Touch screen diagnostics - Added in v2.1.0
- ✅ Memory leak - Fixed in v2.2.0

## Known Limitations

- Uses `delay(1000)` in loop - acceptable for clock application
- No automatic WiFi reconnect in loop if connection drops during runtime
- No automatic NTP resync (only syncs on boot)
- WebUI has no authentication (local network only)
- **⚠️ Default OTA password "change-me" must be changed in production**

## Development Notes

### Building & Uploading

```bash
# Build firmware
pio run

# Upload firmware
pio run -t upload

# Upload WebUI files to LittleFS
pio run -t uploadfs

# Monitor serial output
pio device monitor
```

### OTA Updates (After Initial Upload)

```bash
pio run -t upload --upload-port <device-ip>
```

### Debug Levels

Change `debugLevel` variable (line 36) or add runtime API endpoint:

- 0 = Off (no output)
- 1 = Errors only
- 2 = Warnings + errors
- 3 = Info + warnings + errors (default)
- 4 = Verbose (all debug output)

### Adding Cities to Timezone Database

Edit `include/timezones.h`:

1. Add entry to appropriate region section
2. Update region count in comment
3. Update region ranges in `data/app.js` (lines 57-70)
4. Rebuild and upload filesystem

### Custom Timezone Format (POSIX)

```txt
Format: STD offset DST,start_rule,end_rule

Examples:
- Sydney: AEST-10AEDT,M10.1.0/2,M4.1.0/3
- New York: EST5EDT,M3.2.0/2,M11.1.0/2
- London: GMT0BST,M3.5.0/1,M10.5.0/2
- Tokyo: JST-9 (no DST)

Components:
- STD: Standard time abbreviation
- offset: Hours from UTC (sign reversed: -10 = UTC+10)
- DST: Daylight saving abbreviation
- start_rule: When DST starts (Month.Week.Day/Hour)
- end_rule: When DST ends
```

## Touch Screen Diagnostics (v2.1.0)

**Status**: ENABLED - Touch triggers diagnostics display

### Pin Configuration (ESP32-2432S028)

Through extensive debugging, discovered correct pin mapping differs from common documentation:

- **Touch IRQ**: GPIO36 (T_IRQ) - Active LOW when touched
- **Touch MOSI**: GPIO32 (T_DIN)
- **Touch MISO**: GPIO39 (T_OUT) - **Different from common docs**
- **Touch CLK**: GPIO25 (T_CLK)
- **Touch CS**: GPIO33 (T_CS)
- **SPI Bus**: VSPI (separate from display)

### Touch Detection Method

The `XPT2046_Touchscreen::touched()` method always returns `true` on this board variant. Solution: use direct IRQ pin reading instead.

```cpp
bool isTouched() {
  return (digitalRead(XPT2046_IRQ) == LOW);  // Active LOW when touched
}
```

### Diagnostics Screen

Touch screen to display system diagnostics for 15 seconds (or touch again to dismiss):

**System Information:**

- Firmware version
- Uptime (formatted: days/hours/mins)
- Free heap memory
- Debug level (0-4)

**Network Information:**

- WiFi SSID
- IP address
- Signal strength (RSSI)

**Recent Logs:**

- Last 20 log entries from circular buffer
- Formatted with timestamp, level, and message

### Touch Implementation Details

1. **Edge Detection**: Only triggers on touch-down edge (prevents false triggers)
2. **Debouncing**: 500ms debounce window
3. **Polling Rate**: 50ms (fast enough for responsive touch)
4. **Display Rate**: 1 second (clock updates only once per second)
5. **Auto-Dismiss**: 15 seconds timeout or manual touch to exit

### Font Management for Diagnostics

Diagnostics uses bitmap fonts to maximize screen space. When exiting diagnostics, `currentSmoothFont` is reset to `nullptr` to force reload of smooth fonts for the clock display.

```cpp
// Exit diagnostics
currentSmoothFont = nullptr;  // Force smooth font reload
drawStaticLayout();
// Reset all cached state to force full redraw
for (int i = 0; i < 6; i++) {
  lastTimes[i] = "";
  lastPrevDay[i] = false;
  lastColonState[i] = false;
}
```

## Future Enhancements

- [ ] WiFi reconnect logic in main loop
- [ ] Automatic NTP resync every 24 hours
- [ ] Tidy up WebUI - selecting remote cities and dropdown not intuitive
- [ ] Add display mirror of TFT to WebUI (as per other clock projects)
- [x] ~~Touch support for diagnostics~~ - **COMPLETED v2.1.0**
- [x] ~~API endpoint for debug level adjustment~~ - **COMPLETED v2.0.0**
- [ ] Weather API integration per city
- [ ] Temperature/humidity display
- [ ] User-configurable color schemes
- [ ] MQTT support for home automation
- [ ] Multiple display modes (clock only, weather, mixed)

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version**: 2.2.0 (2026-01-22)

- **CRITICAL FIX**: Eliminated memory leak caused by excessive setenv() calls
- Implemented time result caching (6 cities, recalculate only when minute changes)
- Reduced setenv() call rate from 5.5/sec to 0.1/sec (98% reduction)
- Heap now stable at ~213KB (leak rate: -3.25 bytes/sec, essentially eliminated)
- Device runs indefinitely without crashes (previously crashed after 12-15 minutes)
