# Project: CYD Family Clock

## Overview

World clock displaying current time across 6 configurable timezones (1 home + 5 remote cities) on a 2.8" ILI9341 TFT display. Features dual display modes (portrait digital / landscape with analogue clock), environmental sensor support (BMP280/BME280/SHT3X/HTU21D), web-based configuration with live clock mirror, NVS persistent storage, OTA updates, custom timezone entry, 5-level debug system, and anti-flicker selective redraws with smooth fonts from LittleFS.

## Hardware

- MCU: ESP32 (ESP32-2432S028 "CYD" board)
- Display: ILI9341 2.8" TFT (240x320 portrait / 320x240 landscape)
- Touch: XPT2046 resistive touch screen
- LDR: Light Dependent Resistor for ambient light sensing
- I2C Sensors (optional): BMP280, BME280, SHT3X, or HTU21D on GPIO 22/27
- Power: USB 5V via CYD board

## Build Environment

- Framework: Arduino
- Platform: espressif32
- Key Libraries:
  - TFT_eSPI @ ^2.5.43 (display driver)
  - WiFiManager @ ^2.0.16-rc.2 (credential portal)
  - ArduinoJson @ ^7.0.4 (web API JSON handling)
  - XPT2046_Touchscreen (touch screen driver)
  - Adafruit BMP280 Library @ ^2.6.8 (temperature + pressure sensor)
  - Adafruit BME280 Library @ ^2.2.4 (temperature + humidity + pressure sensor)
  - Adafruit SHT31 Library @ ^2.2.2 (temperature + humidity sensor)
  - Adafruit HTU21DF Library @ ^1.1.0 (temperature + humidity sensor)
  - Adafruit Unified Sensor @ ^1.1.14 (sensor abstraction layer)
  - Built-in: WiFi, ArduinoOTA, WebServer, Preferences (NVS), LittleFS, time.h (NTP)

## Project Structure

```txt
├── src/
│   └── main.cpp              # Main implementation (~2600 lines)
├── include/
│   ├── User_Setup.h          # TFT_eSPI hardware config
│   ├── config.h              # Hardware configuration (sensor selection, I2C pins)
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
| I2C SDA  | 27   | I2C data (sensors)       |
| I2C SCL  | 22   | I2C clock (sensors)      |

## Configuration System

### Persistent Storage (NVS)

Config stored in ESP32 NVS (Non-Volatile Storage) via Preferences API:

- **Namespace**: `"worldclock"`
- **Keys**: `homeLabel`, `homeTz`, `remote0Label`, `remote0Tz`, ..., `remote4Label`, `remote4Tz`, `landscape`, `flip`, `fahrenheit`, `screenRot`, `flipInt`
- **Load/Save**: `loadConfig()` / `saveConfig()` functions

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
FIRMWARE_VERSION = "2.5.0"
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

## Current State (v2.7.0)

### Features

- **6 Cities**: 1 home (reference timezone) + 5 remote cities
- **Dual Display Modes**: Portrait (240x320) or Landscape (320x240) with analogue clock
- **Portrait Screen Rotation**: Auto-flip between standard and alternate portrait screens (with sensor)
- **Alternate Portrait Layout**: Analogue clock + environmental data + compact city list
- **Flip Display**: 180° rotation for mounting with USB on either side
- **Analogue Clock**: Real-time clock face with hour, minute, and second hands (landscape + alternate portrait)
- **Web Configuration**: Full WebUI at `http://<device-ip>` with live clock mirror
- **Screenshot Capture**: Download true TFT pixels as BMP via WebUI button
- **Display Mode Toggle**: Switch between portrait/landscape via WebUI
- **Screen Rotation Control**: Enable/disable and adjust flip interval (3-30 seconds)
- **Persistent Config**: NVS storage, survives reboots (including display mode, flip, and rotation settings)
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
- **Next Day Indicator**: Cyan "NEXT DAY" for cities ahead of home city
- **LDR Support**: Ambient light sensing (for future brightness control)
- **Environmental Sensors**: Optional I2C sensor support (BMP280, BME280, SHT3X, HTU21D)
- **Temperature Display**: Real-time temperature with Celsius/Fahrenheit toggle
- **Humidity Display**: Relative humidity percentage (when sensor supports it)
- **Pressure Display**: Barometric pressure in hPa (when sensor supports it)
- **Sensor Auto-Detection**: Automatic I2C sensor detection at boot

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

### Display Layout - Alternate Portrait Mode (240x320)

**Note**: Alternates with standard portrait when environmental sensor available and screen rotation enabled.

```txt
┌─────────────────────────┐
│   Home: SYDNEY          │ ← Home city label (cyan, centered, y=4)
│                         │
│               06:27     │ ← Home time (centered, y=30)
│                         │
│                         │
│  ╭─────╮   Temp: 25C    │ ← Analogue clock (left)
│ /12  •  \  Hum: 65%     │   Center: (60, 80)
││9  •───3│  Press: 1013  │   + Sensor data (right, y=70)
│ \ 6    /                │   Clock radius: 55px
│  ╰─────╯                │
│                         │
│                         │
├─────────────────────────┤ ← Separator (y=137)
│ VANCOUVER      11:27    │ ← Remote cities
│ Prev Day                │   (no separator lines)
│                         │   Day indicator directly
│ LONDON         19:27    │   below city name
│ Prev Day                │
│                         │
│ NAIROBI        22:27    │
│ Prev Day                │
│                         │
│ DENVER         12:27    │
│ Prev Day                │
│                         │
│ WELLINGTON     08:27    │
│                         │
└─────────────────────────┘
```

**Key Features**:
- Home time prominently displayed at top, centered (y=30)
- Analogue clock positioned to avoid cropping (bottom edge at y=135)
- Sensor data positioned below home time (y=70) to avoid overlap
- Remote cities at y=137 with no separator lines between them
- Day indicators ("Prev Day"/"Next Day") directly below city name, matching standard portrait
- Sensor data always displayed (shows "n/a" when sensor unavailable or metric not supported)
- Humidity shown for all cases (even BMP280 which doesn't have humidity sensor)
- Pressure value shown without "hPa" suffix to prevent wrapping

**Screen Rotation Behavior**:
- Alternates between standard and alternate layouts every N seconds (default: 8)
- Only active when: Portrait mode + Sensor available + Rotation enabled
- Configurable interval: 3-30 seconds via WebUI
- Smooth transitions with state preservation

### Display Layout - Landscape Mode (320x240)

```txt
┌────────────┬────────────────────────────┐
│WORLD CLOCK │ VANCOUVER        20:23     │
│TUE 28 JAN  │   PREV DAY                 │
│            │                            │
│    ╭───╮   │ LONDON           04:23     │
│   /  |  \  │                            │
│  │   •───│ │ NAIROBI          07:23     │
│   \     /  │                            │
│    ╰───╯   │ DENVER           21:23     │
│            │   PREV DAY                 │
│  SYDNEY    │                            │
│   HOME     │ TOKYO            13:23     │
│   14:23    │                            │
│ 25° 1013hPa│                            │ ← Environmental data (if sensor present)
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
- **Environmental Data in Mirror**: Shows sensor readings in landscape and alternate portrait mirrors
- **Alternate Portrait Mirror**: Renders analogue clock layout when active on device
- **Screenshot Capture**: "Capture Screenshot" button downloads true TFT pixels as BMP
- **Display Mode Toggle**: Switch between portrait and landscape modes
- **Flip Display**: Checkbox to flip display 180° (USB on opposite side)
- **Screen Rotation Control**: Enable/disable portrait screen rotation with configurable interval (3-30 sec)
- **Timezone Dropdowns**: 102 predefined cities grouped by region
- **Custom Entry**: Manual POSIX timezone string input
- **Live Preview**: Shows timezone string for selected city
- **System Status**: Firmware, hostname, WiFi, IP, uptime, free heap, LDR value, sensor data
- **Environmental Data**: Displays sensor type, temperature (°C/°F), humidity, pressure
- **Temperature Unit Toggle**: Checkbox to switch between Celsius and Fahrenheit
- **Debug Level Control**: Adjust logging verbosity via dropdown
- **System Actions**: Reboot device, reset WiFi credentials
- **Auto-refresh**: Status updates every 5 seconds

### REST API Endpoints

```txt
GET  /api/state       - System status + current config (JSON, includes flipDisplay)
GET  /api/mirror      - Current clock display data for all cities (JSON)
GET  /api/snapshot    - Download TFT display as BMP image (waits for colons visible)
GET  /api/timezones   - List of 102 predefined timezones (JSON)
POST /api/config      - Update timezone configuration, display mode, and flip setting
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
  bool flipDisplay;            // Flip display 180°: allows USB on opposite side
  bool useFahrenheit;          // Temperature unit: false = Celsius, true = Fahrenheit
};
```

### State Caching (lines 552-566)

Minimizes TFT writes and prevents flicker:

```cpp
char lastDate[16];         // Last drawn date
char lastTimes[6][8];      // Last drawn time for each city ("HH:MM")
bool lastPrevDay[6];       // Last "PREV DAY" state
bool lastNextDay[6];       // Last "NEXT DAY" state
bool lastColonState[6];    // Last colon blink state

// Analogue clock state (landscape mode)
int lastSecond = -1;       // Last drawn second hand position
int lastMinute = -1;       // Last drawn minute hand position
int lastHour = -1;         // Last drawn hour hand position
```

### Selective Redraw Logic

- **Date**: Only redraws when string changes (once per day)
- **Time**: Full time string redraw for colon blink (HH:MM or HH MM)
- **Colon**: Blinks by redrawing full time string with ":" or " " (fixes alignment issues)
- **Prev/Next Day**: Only redraws when day boundary crossed
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

- **Hybrid font system (v2.7.0+)**: Bitmap fonts for static elements, smooth fonts for dynamic time
  - **Bitmap fonts** (5-10× faster rendering):
    - Title, city labels, HOME indicator, date
    - Environmental data labels
    - Static headers
    - Uses `tft.setTextFont(2)` or `tft.setTextFont(4)` directly
  - **Smooth fonts** (antialiased, slower but better looking):
    - Time displays (HH:MM) - most important visual element
    - Day indicators (Prev Day / Next Day)
    - Uses `setFont(smoothName, fallbackSize)` helper
  - **Result**: Near-instant screen redraws (was ~3 seconds) while keeping time display crisp
- Loads smooth fonts from LittleFS on-demand
- Unloads previous font when switching
- Falls back to bitmap fonts if .vlw files missing
- Helper: `setFont(smoothName, fallbackSize)`
- **Font batching optimization**:
  - Batched drawing by font type (10 switches → 2 switches)
  - Alternate portrait: Collect city data first, then draw in 3 font-batched passes
  - Landscape static: Draw cities in 2 passes by name length instead of switching per city
  - Regular updates: Fonts loaded once per update cycle

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
- ✅ NEXT DAY indicator - Added in v2.5.0
- ✅ Screenshot capture via WebUI - Added in v2.5.0

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

## Environmental Sensor Support

### Supported Sensors

Four I2C sensor types are supported via conditional compilation in [include/config.h](include/config.h):

| Sensor | Manufacturer | Measurements | I2C Address |
|--------|-------------|--------------|-------------|
| BMP280 | Bosch | Temperature + Pressure | 0x76 or 0x77 |
| BME280 | Bosch | Temperature + Humidity + Pressure | 0x76 or 0x77 |
| SHT3X | Sensirion | Temperature + Humidity | 0x44 or 0x45 |
| HTU21D | TE Connectivity | Temperature + Humidity | 0x40 |

### Hardware Configuration

#### I2C Pins (CYD Temp/Humidity Interface)
- **SDA**: GPIO 27
- **SCL**: GPIO 22
- **Update Interval**: 10 seconds (configurable in config.h)

#### Sensor Selection

Edit [include/config.h](include/config.h) and uncomment ONE sensor type:

```cpp
#define USE_BMP280   // Bosch BMP280 - Temperature + Pressure
// #define USE_BME280   // Bosch BME280 - Temperature + Humidity + Pressure
// #define USE_SHT3X    // Sensirion SHT3X - Temperature + Humidity
// #define USE_HTU21D   // TE HTU21D - Temperature + Humidity

#define SENSOR_SDA_PIN  27
#define SENSOR_SCL_PIN  22
#define SENSOR_UPDATE_INTERVAL 10000  // 10 seconds
```

### Auto-Detection

Sensors are automatically detected at boot via I2C scan:

```cpp
bool testSensor() {
  Wire.begin(SENSOR_SDA_PIN, SENSOR_SCL_PIN);

  #ifdef USE_BMP280
    if (bmp280.begin(0x76)) {
      sensorType = "BMP280";
      sensorAvailable = true;
      return true;
    }
  #endif
  // ... similar for other sensor types

  return false;
}
```

### Data Reading

`updateSensorData()` function reads sensor data with conditional compilation:

```cpp
bool updateSensorData() {
  if (!sensorAvailable) return false;

  #ifdef USE_BMP280
    temperature = bmp280.readTemperature();
    pressure = bmp280.readPressure() / 100.0;  // Convert Pa to hPa
  #elif defined(USE_BME280)
    temperature = bme280.readTemperature();
    humidity = bme280.readHumidity();
    pressure = bme280.readPressure() / 100.0;
  // ... etc
  #endif

  return true;
}
```

### TFT Display

Environmental data appears in multiple display modes:

**Landscape Mode** (left panel, y=218):
- **BME280**: `25°C 65% 1013hPa` (temp, humidity, pressure)
- **BMP280**: `25°C  1013hPa` (temp, pressure)
- **SHT3X/HTU21D**: `25°C  65%` (temp, humidity)

**Alternate Portrait Mode** (right side, y=70-98):
```
Temp: 25°C
Hum: 65%
Press: 1013hPa
```

**Styling:**
- Font: `kFontNote` (NotoSans-Bold7)
- Temperature: Color-coded based on value (see Temperature Color Coding)
- Humidity/Pressure: Light grey (TFT_LIGHTGREY)
- Units: °C/°F for temperature, % for humidity, hPa for pressure
- Negative temperatures: Displayed with "-" prefix (e.g., "-5°C")
- Visibility: Landscape mode (when sensor available), Alternate portrait (always, shows "n/a" when unavailable)

### WebUI Display

System Status section shows:
- **Sensor Type**: BMP280, BME280, SHT3X, HTU21D, or N/A
- **Temperature**: Real-time with unit (°C or °F)
- **Humidity**: Percentage (when available)
- **Pressure**: hPa (when available)
- **Temperature Toggle**: Checkbox to switch °C ↔ °F

### Temperature Unit Conversion

Celsius ↔ Fahrenheit conversion:

```cpp
// Display temperature
int displayTemp = config.useFahrenheit
  ? (int)(temperature * 9.0 / 5.0 + 32)  // Convert to Fahrenheit
  : (int)temperature;                      // Keep as Celsius

// API response (server-side conversion)
if (config.useFahrenheit) {
  tempValue = (int)(temperature * 9.0 / 5.0 + 32);
  unit = "°F";
} else {
  tempValue = (int)temperature;
  unit = "°C";
}
```

Unit preference stored in NVS (`PREF_FAHRENHEIT` key).

### Temperature Color Coding

Temperature values are color-coded based on user-definable ranges for visual feedback:

**Default Temperature Ranges (in Celsius):**
- **Freezing** (≤ 0°C): Blue (`TFT_BLUE`) - Below or at freezing point
- **Cold** (1-15°C): Cyan (`TFT_CYAN`) - Cold but above freezing
- **Pleasant** (16-25°C): Green (`TFT_GREEN`) - Comfortable temperature
- **Hot** (26-35°C): Orange (`TFT_ORANGE`) - Hot weather
- **Extreme** (> 35°C): Red (`TFT_RED`) - Extreme heat

**Customization:**

Edit temperature thresholds in [src/main.cpp](src/main.cpp:577-597):

```cpp
// Temperature color coding - User Definable Ranges (in Celsius)
#define TEMP_FREEZING_MAX 0      // <= 0°C: Freezing (Blue)
#define TEMP_COLD_MAX 15         // 1-15°C: Cold (Cyan)
#define TEMP_PLEASANT_MAX 25     // 16-25°C: Pleasant (Green)
#define TEMP_HOT_MAX 35          // 26-35°C: Hot (Orange)
                                 // > 35°C: Extreme (Red)

// Temperature display colors
#define COLOR_TEMP_FREEZING TFT_BLUE
#define COLOR_TEMP_COLD     TFT_CYAN
#define COLOR_TEMP_PLEASANT TFT_GREEN
#define COLOR_TEMP_HOT      TFT_ORANGE
#define COLOR_TEMP_EXTREME  TFT_RED
```

**Features:**
- Color determination always uses Celsius for consistency
- Works with both Celsius and Fahrenheit display modes
- Handles negative temperatures correctly (shows "-" prefix)
- Temperature displayed with degree symbol (°C or °F)
- Pressure displayed with "hPa" unit

### Serial Output

Sensor readings logged every 10 seconds:

```
[INFO] Sensor: BMP280 - 24.5°C, 1013.2 hPa
[INFO] Sensor: BME280 - 23.8°C, 62.3%, 1011.5 hPa
```

### API Integration

`/api/state` endpoint includes sensor data:

```json
{
  "sensorAvailable": true,
  "sensorType": "BMP280",
  "temperature": 24,
  "humidity": null,
  "pressure": 1013,
  "useFahrenheit": false
}
```

`/api/config` accepts temperature unit updates:

```json
POST /api/config
{
  "useFahrenheit": true
}
```

## Future Enhancements

- [ ] WiFi reconnect logic in main loop
- [ ] Automatic NTP resync every 24 hours
- [ ] Automatic brightness control using LDR
- [ ] Touch-based city selection/editing
- [x] ~~Touch support for diagnostics~~ - **COMPLETED v2.1.0**
- [x] ~~API endpoint for debug level adjustment~~ - **COMPLETED v2.0.0**
- [x] ~~Display mirror of TFT to WebUI~~ - **COMPLETED v2.4.0**
- [x] ~~Multiple display modes~~ - **COMPLETED v2.4.0** (portrait/landscape)
- [x] ~~Temperature/humidity display~~ - **COMPLETED v2.6.1** (I2C sensor support)
- [x] ~~Environmental data display in portrait mode~~ - **COMPLETED v2.7.0** (alternate screen with rotation)
- [ ] Weather API integration per city
- [ ] User-configurable color schemes
- [ ] MQTT support for home automation

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version**: 2.8.0 (2026-02-01)

- **Hybrid font system** - 5-10× faster screen redraws (bitmap for static, smooth for time)
- **Font batching optimization** - 80% reduction in font switches during mode transitions
- **Temperature color coding** - User-definable ranges (freezing/cold/pleasant/hot/extreme)
- **Enhanced environmental display** - Unit symbols added (°C/°F, hPa, %) with negative temperature support
- **Improved alternate portrait** - Refined layout with better positioning and spacing
- Portrait screen rotation - Auto-flip between standard and alternate portrait screens
- Alternate portrait layout - Analogue clock + environmental data + compact city list
- Screen rotation control - Enable/disable toggle with configurable interval (3-30 seconds)
- WebUI mirror support - Renders both standard and alternate portrait layouts
- Environmental sensor support - BMP280, BME280, SHT3X, HTU21D sensors
- WebUI mirror integration - Environmental data shown in landscape and alternate portrait
- Temperature unit toggle - Celsius/Fahrenheit switching in WebUI
- Flip display - 180° rotation for mounting with USB on either side
- BMP screenshot format - replaced JPEG for better reliability, ~230KB files
- NEXT DAY indicator (cyan) for cities ahead of home city
- Landscape mode with analogue clock display
- Live clock mirror in WebUI (2-second refresh)
- Eliminated memory leak (v2.3.0) - heap stable indefinitely
