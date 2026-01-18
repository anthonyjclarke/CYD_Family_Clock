# Changelog

All notable changes to the CYD World Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
