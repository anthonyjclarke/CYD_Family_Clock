# Project: CYD Family Clock

## Overview

World clock displaying current time across 6 configurable timezones (1 home + 5 remote cities) on a 2.8" ILI9341 TFT display. Features dual display modes (portrait digital / landscape with analogue clock), web-based configuration with live clock mirror, NVS persistent storage, OTA updates, custom timezone entry, 5-level debug system, and anti-flicker selective redraws with smooth fonts from LittleFS.

## Hardware

- MCU: ESP32 (ESP32-2432S028 "CYD" board)
- Display: ILI9341 2.8" TFT (240x320 portrait / 320x240 landscape)
- Touch: XPT2046 resistive touch screen
- LDR: Light Dependent Resistor for ambient light sensing
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
│   └── main.cpp              # Main implementation (~2100 lines)
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

### Display (ILI9341)

| Function | GPIO | Notes                |
|----------|------|----------------------|
| TFT_MOSI | 13   | SPI data out         |
| TFT_MISO | 12   | SPI data in          |
| TFT_SCLK | 14   | SPI clock            |
| TFT_CS   | 15   | Chip select          |
| TFT_DC   | 2    | Data/command         |
| TFT_RST  | -1   | Tied to ESP32 RST    |
| TFT_BL   | 21   | Backlight (active)   |

### Touch Screen (XPT2046)

| Function   | GPIO | Notes                   |
|------------|------|-------------------------|
| Touch IRQ  | 36   | T_IRQ (active LOW)      |
| Touch MOSI | 32   | T_DIN                   |
| Touch MISO | 39   | T_OUT (board-specific)  |
| Touch CLK  | 25   | T_CLK                   |
| Touch CS   | 33   | T_CS                    |

### Sensors

| Function | GPIO | Notes                    |
|----------|------|--------------------------|
| LDR      | 34   | Analog input (0-4095)    |

## Configuration System

### Persistent Storage (NVS)

Config stored in ESP32 NVS (Non-Volatile Storage) via Preferences API:

- **Namespace**: `"worldclock"`
- **Keys**: `homeLabel`, `homeTz`, `remote0Label`, `remote0Tz`, ..., `remote4Label`, `remote4Tz`, `landscape`
- **Load/Save**: `loadConfig()` / `saveConfig()` functions (lines 446-496)

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

### Network Settings (line 160)

```cpp
FIRMWARE_VERSION = "2.4.0"
OTA_HOSTNAME = "WorldClock"
OTA_PASSWORD = "change-me"  // ⚠️ CHANGE IN PRODUCTION
NTP_SERVER1 = "pool.ntp.org"
NTP_SERVER2 = "time.nist.gov"
```

### Display Layout (lines 508-546)

```cpp
// Colors
COLOR_BG = TFT_BLACK
COLOR_LABEL = TFT_WHITE
COLOR_TIME = TFT_GREEN

// Portrait mode layout (240x320)
kHeaderHeight = 40px        // "WORLD CLOCK" + date
kPad = 8px                  // Left padding
6 cities × 46.7px per row   // Home + 5 remote

// Landscape mode layout (320x240)
kLeftPanelWidth = 120px     // Title, date, clock, home city
kRightPanelWidth = 200px    // 5 remote cities
kLandscapeRemoteRowHeight = 48px  // 240 / 5 = 48px per row

// Analogue clock settings
kClockCenterX = 60          // Center of left panel
kClockCenterY = 95          // Vertical center
kClockRadius = 50           // Clock face radius
kHourHandLen = 25           // Hour hand length
kMinuteHandLen = 35         // Minute hand length
kSecondHandLen = 40         // Second hand length

// Fonts
kFontHeader = "NotoSans-Bold9" (fallback: 2)
kFontLabel = "NotoSans-Bold10" (fallback: 4)
kFontTime = "NotoSans-Bold16" (fallback: 6)
kFontNote = "NotoSans-Bold7" (fallback: 2)
```

## Current State (v2.4.0)

### Features

- **6 Cities**: 1 home (reference timezone) + 5 remote cities
- **Dual Display Modes**: Portrait (240x320) or Landscape (320x240) with analogue clock
- **Analogue Clock**: Real-time clock face with hour, minute, and second hands
- **Web Configuration**: Full WebUI at `http://<device-ip>` with live clock mirror
- **Display Mode Toggle**: Switch between portrait/landscape via WebUI
- **Persistent Config**: NVS storage, survives reboots (including display mode)
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
- **LDR Support**: Ambient light sensing (for future brightness control)

### Display Layout - Portrait Mode (240x320)

```txt
┌─────────────────────────┐
│   WORLD CLOCK           │ ← Title (white)
│   TUE 28 JAN            │ ← Date from home city
├─────────────────────────┤
│ SYDNEY         14:23    │ ← Home city (green time)
│   HOME                  │ ← Cyan indicator
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

### Display Layout - Landscape Mode (320x240)

```txt
┌────────────┬────────────────────────────┐
│WORLD CLOCK │ VANCOUVER        20:23    │
│TUE 28 JAN  │   PREV DAY                │
├────────────┼────────────────────────────┤
│    ╭───╮   │ LONDON           04:23    │
│   /  |  \  │                           │
│  │   •───│ │ NAIROBI          07:23    │
│   \     /  │                           │
│    ╰───╯   │ DENVER           21:23    │
│            │   PREV DAY                │
│  SYDNEY    ├────────────────────────────┤
│   HOME     │ TOKYO            13:23    │
│   14:23    │                           │
└────────────┴────────────────────────────┘
 Left Panel     Right Panel (5 remote cities)
  (120px)              (200px)
```

### Analogue Clock Details

- **Hour markers**: 12 positions, thicker at 12/3/6/9
- **Hour hand**: White, 3px thick, 25px length, moves with minutes
- **Minute hand**: White, 2px thick, 35px length
- **Second hand**: Red, 1px thick, 40px length
- **Center dot**: White, 3px radius
- **Face circle**: Dark grey outline

### WebUI Features

- **Live Clock Display**: Real-time mirror of all city times (updates every 2 seconds)
- **Display Mode Toggle**: Switch between portrait and landscape modes
- **Timezone Dropdowns**: 102 predefined cities grouped by region
- **Custom Entry**: Manual POSIX timezone string input
- **Live Preview**: Shows timezone string for selected city
- **System Status**: Firmware, hostname, WiFi, IP, uptime, free heap, LDR value
- **Debug Level Control**: Adjust logging verbosity via dropdown
- **System Actions**: Reboot device, reset WiFi credentials
- **Auto-refresh**: Status updates every 5 seconds

### REST API Endpoints

```txt
GET  /api/state       - System status + current config (JSON)
GET  /api/mirror      - Current clock display data for all cities (JSON)
GET  /api/timezones   - List of 102 predefined timezones (JSON)
POST /api/config      - Update timezone configuration and display mode
POST /api/debug-level - Change debug level at runtime
POST /api/reboot      - Reboot device
POST /api/reset-wifi  - Clear WiFi credentials and reboot to AP mode
GET  /                - Web UI (index.html)
GET  /app.js          - WebUI JavaScript
GET  /style.css       - WebUI styling
```

## Architecture Notes

### Config Structure (lines 164-172)

```cpp
struct Config {
  char homeCityLabel[32];      // Home city name
  char homeCityTz[64];         // Home POSIX timezone
  char remoteCities[5][32];    // 5 remote city names
  char remoteTzStrings[5][64]; // 5 remote POSIX timezones
  bool landscapeMode;          // Display orientation: true = landscape
};
```

### State Caching (lines 552-566)

Minimizes TFT writes and prevents flicker:

```cpp
char lastDate[16];         // Last drawn date
char lastTimes[6][8];      // Last drawn time for each city ("HH:MM")
bool lastPrevDay[6];       // Last "PREV DAY" state
bool lastColonState[6];    // Last colon blink state

// Analogue clock state (landscape mode)
int lastSecond = -1;       // Last drawn second hand position
int lastMinute = -1;       // Last drawn minute hand position
int lastHour = -1;         // Last drawn hour hand position
```

### Selective Redraw Logic

- **Date**: Only redraws when string changes (once per day)
- **Time**: Only redraws when HH:MM changes (once per minute)
- **Colon**: Targeted 1-char redraw every second (blinking animation)
- **Prev Day**: Only redraws when day boundary crossed
- **Labels**: Only redrawn during `drawStaticLayout()` after config change

### Timezone Calculation (No Memory Leak)

Uses manual POSIX timezone parsing instead of `setenv()` to avoid memory leaks:

```cpp
// Parse timezone once at config load
ParsedTimezone parsedTz[6];
parseTimezoneString(config.homeCityTz, &parsedTz[0]);

// Get local time without setenv - NO MEMORY LEAK
void getLocalTimeNoSetenv(time_t utc, const ParsedTimezone* tz, struct tm* out) {
  int16_t offsetMins = isDstActive(utc, tz) ? tz->dstOffsetMins : tz->stdOffsetMins;
  time_t local = utc + (offsetMins * 60);
  gmtime_r(&local, out);  // No setenv needed!
}
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

### Main Loop

```cpp
loop() {
  ArduinoOTA.handle();      // Handle OTA updates
  server.handleClient();    // Handle web requests

  // Touch handling (50ms polling)
  if (millis() - lastTouchPoll >= 50) {
    handleTouch();
    lastTouchPoll = millis();
  }

  // Display update (1 second interval)
  if (millis() - lastDisplayUpdate >= 1000) {
    if (showingDiagnostics) {
      checkDiagnosticsTimeout();
    } else {
      drawTimes();  // Calls drawTimesPortrait() or drawTimesLandscape()
    }
    lastDisplayUpdate = millis();
  }
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

- **Flash**: ~1,050,000 bytes (~80% of 1.3MB)
- **RAM**: ~54,000 bytes (~16.5% of 320KB)
- **Heap**: Stable at ~200KB+ (no memory leak)

## WiFi Behavior

- **First boot**: Creates AP "WorldClock-Setup", captive portal
- **Configured**: Auto-connects to saved network
- **Failed connect**: Fallback AP "WorldClock-AP"
- **Timeouts**: Portal stays open indefinitely, 20s connect attempt

## Critical Fix: Memory Leak (v2.2.0 → v2.3.0)

### Problem
Device was experiencing memory leak, causing eventual crashes. The ESP32's `setenv("TZ", ...)` function leaks ~30-40 bytes per call.

### v2.2.0 Partial Fix
Implemented time caching to reduce `setenv()` calls from ~300/minute to 6/minute. This reduced the leak from -186 bytes/sec to -3 bytes/sec, but still leaked ~11KB/hour.

### v2.3.0 Complete Fix
**Eliminated setenv() entirely** by implementing manual POSIX timezone parsing:

- Parse timezone strings once at config load (not every time calculation)
- Calculate UTC offset and DST transitions manually
- Use `gmtime_r()` instead of `localtime_r()` with manual offset
- **Zero setenv() calls in main loop = Zero memory leak**

### Implementation Details
```cpp
// Parsed timezone structure (replaces setenv)
struct ParsedTimezone {
  int16_t stdOffsetMins;   // Standard time offset from UTC
  int16_t dstOffsetMins;   // DST offset from UTC
  bool hasDst;
  DstRule dstStart, dstEnd; // DST transition rules
};

static ParsedTimezone parsedTz[6];  // Parsed once at config load

// Get local time without setenv - NO MEMORY LEAK
void getLocalTimeNoSetenv(time_t utc, const ParsedTimezone* tz, struct tm* out) {
  int16_t offsetMins = isDstActive(utc, tz) ? tz->dstOffsetMins : tz->stdOffsetMins;
  time_t local = utc + (offsetMins * 60);
  gmtime_r(&local, out);  // No setenv needed!
}
```

**Result:** Heap completely stable. Device runs indefinitely without memory degradation.

## Resolved Issues from v1.0.0

- ✅ Config persistence - Now stored in NVS
- ✅ Web interface - Full WebUI for configuration
- ✅ OTA support - Wireless updates with progress bar
- ✅ Runtime city selection - No recompilation needed
- ✅ Custom timezones - Manual POSIX string entry
- ✅ WiFi reconnect - WiFiManager handles credentials
- ✅ 6th city support - Added in v2.0.0
- ✅ Touch screen diagnostics - Added in v2.1.0
- ✅ Memory leak - Reduced in v2.2.0, **eliminated in v2.3.0**
- ✅ Display mirror in WebUI - Added in v2.4.0
- ✅ Multiple display modes - Portrait/Landscape added in v2.4.0

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

## Analogue Clock Implementation (v2.4.0)

### Clock Face Drawing

`drawAnalogClockFace()` draws the static elements:
- Circle outline (dark grey, 50px radius)
- 12 hour markers (thicker at 12/3/6/9)
- Center dot (white, 3px)

### Hand Drawing

`drawClockHand(cx, cy, length, angleDeg, color, thickness)`:
- Converts angle to radians (0° = 12 o'clock, clockwise)
- Draws line from center to calculated endpoint
- Supports variable thickness via parallel lines

### Selective Redraw

`updateAnalogClockHands(hour, minute, second)`:
1. Calculate new hand angles
2. Erase old hands (draw in background color)
3. Draw new hands (hour → minute → second order)
4. Redraw center dot

### Angle Calculations

```cpp
// Hour: 30° per hour + 0.5° per minute (smooth movement)
float hourAngle = (hour % 12) * 30.0f + minute * 0.5f;
// Minute: 6° per minute
float minuteAngle = minute * 6.0f;
// Second: 6° per second
float secondAngle = second * 6.0f;
```

## LDR (Light Sensor) Support

### Hardware
- Connected to GPIO34 (analog input)
- ADC range: 0-4095 (12-bit)
- 11dB attenuation for 0-3.3V range

### Reading

```cpp
int readLDR() {
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LDR_PIN);
    delay(1);
  }
  return sum / 10;  // 10-sample average
}
```

### Current Use
- Value displayed in WebUI System Status
- Future: automatic brightness control

## Future Enhancements

- [ ] WiFi reconnect logic in main loop
- [ ] Automatic NTP resync every 24 hours
- [ ] Automatic brightness control using LDR
- [ ] Touch-based city selection/editing
- [x] ~~Touch support for diagnostics~~ - **COMPLETED v2.1.0**
- [x] ~~API endpoint for debug level adjustment~~ - **COMPLETED v2.0.0**
- [x] ~~Display mirror of TFT to WebUI~~ - **COMPLETED v2.4.0**
- [x] ~~Multiple display modes~~ - **COMPLETED v2.4.0** (portrait/landscape)
- [ ] Weather API integration per city
- [ ] Temperature/humidity display
- [ ] User-configurable color schemes
- [ ] MQTT support for home automation

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version**: 2.4.0 (2026-01-28)

- **Landscape mode** with analogue clock display
- **Display mode toggle** via WebUI (portrait/landscape)
- **Live clock mirror** in WebUI (2-second refresh)
- **LDR support** for ambient light sensing
- **Flicker-free clock hands** with selective redraw
- **Persistent display mode** in NVS storage
- Eliminated memory leak (v2.3.0) - heap stable indefinitely
