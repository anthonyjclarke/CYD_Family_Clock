# Changelog

All notable changes to the CYD World Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.2.0] - 2026-01-22

### Fixed - Critical Memory Leak

#### Problem Identified
- **Severe memory leak** causing device crashes after 12-15 minutes
- Heap dropping at rate of **~186 bytes/second** (from 200KB to exhaustion)
- Root cause: ESP32's `setenv("TZ")` function leaks ~40 bytes per call
- Clock was calling `setenv()` **4-6 times per second** while cycling through 6 timezones

#### Solution Implemented
- **Time result caching** in `formatTime()` function
- Cache stores formatted time for all 6 cities (home + 5 remote)
- Recalculation only occurs when the minute changes
- Reduced `setenv()` calls from **~300/minute to 6/minute** (98% reduction)

#### Results
- **Leak rate reduced** from -186 bytes/sec to -3.25 bytes/sec
- Heap now **stable at ~213KB** with minimal drift
- Device **runs indefinitely** without crashes
- Memory leak effectively eliminated

#### Technical Details
- Added `CachedTimeInfo` structure for each city
- Implemented minute-based cache invalidation
- Preserved all timezone switching for DST accuracy
- Maintained `setenv()/tzset()` caching to minimize redundant calls

### Changed
- Upgraded ArduinoJson from v6.21.3 to v7.0.4
- Updated library compatibility for ArduinoJson v7 syntax
- Changed `containsKey()` to `is<T>()` pattern (v7 deprecation)
- WebUI polling interval remains at 5 seconds (from testing)

## [2.1.0] - 2026-01-19

### Added - Touch Screen Diagnostics

#### Touch Screen Support

- **Touch screen diagnostics display** - Touch screen to view system information
- **15-second auto-dismiss** - Diagnostics screen automatically closes after 15 seconds
- **Manual dismiss** - Touch screen again to immediately close diagnostics
- **Responsive touch polling** - 50ms polling rate for immediate response
- **Edge detection** - Only triggers on touch-down edge to prevent false triggers
- **500ms debouncing** - Prevents accidental double-touches

#### Diagnostics Screen Content

- **System information**:
  - Firmware version
  - Uptime (formatted: days/hours/mins/secs)
  - Free heap memory
  - Current debug level (0-4)
- **Network information**:
  - WiFi SSID
  - IP address
  - Signal strength (RSSI in dBm)
- **Recent logs**:
  - Last 20 log entries from circular buffer
  - Timestamp, level, and message for each entry
  - Chronological order (oldest to newest)

#### Web UI Enhancements

- **Debug level selector** added to System Status section
- **Real-time debug level adjustment** via web interface
- **Success notification** when debug level changed
- **Auto-refresh status** - System status updates every 30 seconds
- **Helper functions** for formatting bytes and uptime

#### API Endpoints

- **POST /api/debug-level** - Change debug level at runtime
- **GET /api/debug** - Return recent logs and debug info (for future use)

### Changed

#### Performance Optimizations

- **Main loop timing** - Changed from 1000ms delay to 50ms polling
- **Display update throttling** - Clock display updates only once per second
- **Touch responsiveness** - Touch detected within 50ms instead of up to 1 second
- **Separate timing** - Touch polling and display updates now independent

#### Serial Output

- **Compressed debug format** - All 6 cities now on single line
- **Pipe separators** - " | " used between cities instead of newlines
- **Format example**: `Nairobi (HOME) 07:24 | Vancouver 20:24 (PREV DAY) | London 04:24 | ...`
- **Reduced verbosity** - Easier to read time progression at a glance

### Fixed

#### Display Issues

- **"PREV DAY" indicator** - Now displays correctly after exiting diagnostics
- **Font reload** - `currentSmoothFont` reset to `nullptr` when exiting diagnostics
- **State cache** - All cached state arrays (`lastTimes`, `lastPrevDay`, `lastColonState`) reset on diagnostics exit
- **Smooth fonts** - Properly reload when returning from bitmap font diagnostics screen

#### Touch Implementation

- **Correct pin mapping** - GPIO36 for IRQ, GPIO39 for MISO (board-specific)
- **IRQ-based detection** - Direct pin reading instead of library's `touched()` method
- **Separate SPI bus** - VSPI for touch, doesn't interfere with display

### Technical Details

#### Dependencies Added

- **XPT2046_Touchscreen** library @ 1.4.0 - Touch screen driver

#### Pin Configuration (ESP32-2432S028)

- Touch IRQ: GPIO36 (T_IRQ, active LOW)
- Touch MOSI: GPIO32 (T_DIN)
- Touch MISO: GPIO39 (T_OUT) - **Different from common documentation**
- Touch CLK: GPIO25 (T_CLK)
- Touch CS: GPIO33 (T_CS)

#### Memory Impact

- **Flash**: 1,028,837 bytes (78.5%) - Increased ~18KB from v2.0.0
- **RAM**: 53,480 bytes (16.3%) - Increased ~2.5KB from v2.0.0

#### Code Statistics

- **main.cpp**: ~1,350 lines (expanded from ~760)
- **New functions**: `drawDiagnosticsScreen()`, `handleTouch()`, `checkDiagnosticsTimeout()`, `isTouched()`
- **New variables**: Circular log buffer (20 entries), diagnostics state tracking

### Documentation

#### Updated Files

- **CLAUDE.md** - Added comprehensive touch screen section with implementation details
- **CHANGELOG.md** - This version's changes
- **data/index.html** - Added debug level selector and helper text
- **data/app.js** - Added debug level handler and formatting helpers

---

## [2.0.0] - 2026-01-18

### Added - Major Feature Update

#### Configuration System

- **Web-based configuration interface** - Configure cities through browser
- **NVS persistent storage** - Timezone settings saved across reboots
- **Dynamic city management** - Home city + 5 remote cities configurable at runtime
- **REST API endpoints**:
  - `GET /api/state` - System status and configuration
  - `POST /api/config` - Update timezone configuration
  - `POST /api/reset-wifi` - Clear WiFi credentials

#### Debug System

- **5-level debug logging** system (Off/Error/Warn/Info/Verbose)
- **Runtime-adjustable** debug levels via `debugLevel` variable
- **Formatted output** with level prefixes: `[ERR]`, `[WARN]`, `[INFO]`, `[VERB]`
- **Legacy compatibility** macros for existing debug statements

#### Network Features

- **Enhanced WiFi setup** with WiFiManager callbacks
- **Fallback AP mode** when WiFiManager fails (CYD-WorldClock-AP)
- **OTA support** for wireless firmware updates
- **On-screen progress bar** during OTA updates
- **WebServer** with static file serving from LittleFS

#### User Interface

- **Startup display** - Boot messages shown on screen during initialization
- **Splash screen** - Globe animation sequence on startup:
  - Title fade-in animation
  - Globe with timezone markers
  - Firmware version display
- **Status indicators** - Color-coded startup steps (green=OK, red=fail, yellow=warning)

#### Display Improvements

- **Centered layout redesign**:
  - "WORLD CLOCK" title centered on row 1
  - Date centered on row 2
  - Cities shifted down to accommodate new layout
- **Better spacing** - Increased header height from 28px to 40px
- **Improved readability** - Separate title and date rows

### Changed

#### Architecture

- **Migrated from hardcoded arrays to Config structure** for dynamic configuration
- **Reference timezone** now uses home city instead of hardcoded Sydney
- **Drawing functions** updated to use lambda helpers for timezone/label access
- **Cached state arrays** changed from `sizeof(locations)` to fixed size `[5]`

#### Code Organization

- **Debug system** replaces old `DEBUG_LOGS` boolean
- **Function names** updated for clarity:
  - `connectWifi()` → `startWifi()`
  - Added `configModeCallback()` for WiFiManager
  - Added `setupOTA()`, `setupWebServer()`, `loadConfig()`, `saveConfig()`

#### Configuration

- **Timezone storage** moved from compile-time to NVS runtime storage
- **NTP sync** now uses home city timezone instead of hardcoded `SYDNEY_TZ`
- **Default cities** maintained for initial configuration

### Fixed

- **Text overlap issues** - Previous "WORLD CLOCK"/date overlap resolved
- **"Vancouver" truncation** - Label clear width adjusted to prevent clipping
- **"PREV DAY" logic** - Now compares to home city instead of hardcoded Sydney
- **Line number offsets** - Updated CLAUDE.md with correct line references

### Technical Details

#### Dependencies Added

- **ArduinoJson** @ ^6.21.3 - For web API (minimal usage with manual parsing)

#### Memory Impact

- **Flash**: 1,010,529 bytes (77.1%) - Increased from ~650KB due to new features
- **RAM**: 51,024 bytes (15.6%) - Well within ESP32 limits

#### Files Added

- `data/index.html` - Web configuration interface (~200 lines)
- `data/app.js` - JavaScript logic (~300 lines)
- `data/style.css` - Dark theme styling (~100 lines)
- `README.md` - Comprehensive documentation
- `CHANGELOG.md` - This file

#### Code Statistics

- **main.cpp**: Expanded from ~452 lines to ~760 lines
- **New functions**: 10+ (config management, web API, OTA, splash, etc.)
- **Build time**: ~31 seconds (successful)

### Security Notes

- ⚠️ **Default OTA password** is "change-me" - **MUST be changed in production**
- **WiFi credentials** stored securely by WiFiManager
- **Web API** has no authentication (local network only)

---

## [1.0.0] - 2026-01-18 (Initial Version)

### Added - Initial Release

#### Core Features

- **World clock display** with 5 hardcoded timezones
- **ILI9341 TFT support** (240x320 resolution)
- **Smooth fonts** from LittleFS with bitmap fallback
- **NTP time sync** on startup
- **WiFiManager** for credential configuration
- **State caching** for flicker-free updates

#### Display Features

- **Centered "WORLD CLOCK" title** and date header
- **5 timezone rows** with city labels and times
- **Blinking colon** animation every second
- **"PREV DAY" indicator** for cities in previous day (yellow)
- **Selective redraws** to minimize TFT writes

#### Hardcoded Configuration

- **Cities**: Sydney, Vancouver, London, Nairobi, Denver
- **Reference timezone**: Sydney (AEST)
- **Colors**: Black background, white labels, green times
- **Fonts**: NotoSans Bold (7/9/10/16 point sizes)

#### Technical Implementation

- **Single-file architecture** (~452 lines)
- **POSIX timezone strings** for DST handling
- **Dynamic timezone switching** via `setenv()`/`tzset()`
- **Serial debug output** at 115200 baud

### Known Limitations (v1.0.0)

- Timezones hardcoded in source
- No configuration persistence
- No web interface
- No OTA support
- WiFi doesn't reconnect after initial setup

---

## Version Numbering

**Format**: MAJOR.MINOR.PATCH

- **MAJOR**: Breaking changes, architectural redesign
- **MINOR**: New features, non-breaking enhancements
- **PATCH**: Bug fixes, documentation updates

---

## Upcoming / Planned Features

### v2.1.0 (Planned)

- [ ] WiFi reconnect logic in main loop
- [ ] Automatic NTP resync every 24 hours
- [ ] Touch support for city selection
- [ ] API endpoint for debug level adjustment
- [ ] Proper ArduinoJson parsing instead of manual string parsing

### v3.0.0 (Future)

- [ ] Weather API integration per city
- [ ] Temperature/humidity display
- [ ] Multiple display modes (clock only, weather, mixed)
- [ ] User-configurable color schemes
- [ ] MQTT support for home automation
- [ ] Configurable NTP servers via web UI

---

## Migration Guide

### v1.0.0 → v2.0.0

**Breaking Changes:**
- Configuration now stored in NVS - first boot will use defaults
- Reference timezone changed from hardcoded Sydney to configurable home city

**Required Actions:**
1. Upload new firmware: `pio run -t upload`
2. Upload web UI files: `pio run -t uploadfs`
3. Access web interface and configure cities
4. **Change OTA password** in `main.cpp` line 70 before deploying!

**Optional:**
- Update `CLAUDE.md` with new line numbers if you've modified it
- Review new debug levels and adjust `DEBUG_LEVEL` if needed

---

**For more information, see [README.md](README.md)**
