# Changelog

All notable changes to the CYD World Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

---

## [2.8.0] - 2026-02-01

### Changed - Display Performance Optimization

#### Hybrid Font System (MAJOR PERFORMANCE FIX)
- **Near-instant screen transitions** - eliminated 3-second delay during mode switching
- **Root cause**: Smooth font rendering from LittleFS is 5-10× slower than bitmap fonts
- **Solution**: Hybrid approach using fast bitmap fonts for static elements, smooth fonts for dynamic time display
- **Bitmap fonts now used for**:
  - Title ("WORLD CLOCK")
  - City labels (all 6 cities)
  - "HOME" indicator
  - Date display
  - Environmental data labels
  - Static headers
- **Smooth fonts still used for**:
  - Time displays (HH:MM) - keeps the most important element looking crisp
  - Day indicators (Prev Day / Next Day)
- **Result**:
  - Screen redraws 5-10× faster (from ~3 seconds to near-instant)
  - Time display retains smooth, antialiased appearance where it matters most
  - Best of both worlds: speed + visual quality

#### Font Loading Optimization (Batching)
- **Batched drawing by font type** - draw all elements needing same font together, then switch once
- **Reduced font switches during mode transitions**:
  - Alternate portrait remote cities: **10 font switches → 2** (80% reduction)
  - Landscape static layout: **Up to 5 font switches → 1** (80% reduction)
- **Technical approach**:
  - Collect what needs drawing in first pass
  - Execute in font-batched passes:
    - Pass 1: Draw all city names
    - Pass 2: Draw all times
    - Pass 3: Draw all day indicators

#### Font Loading Optimization (Regular Updates)
- **Optimized normal clock updates** (colon blink, minute changes) - already fast
- **Eliminated redundant font loading**: Fonts loaded once per update cycle
- **Reduced LittleFS I/O**: Minimized flash storage reads during display updates
- **Optimized all display modes**:
  - Portrait mode: Font loaded before loop, minimal switches for day indicators
  - Landscape mode: Font loaded once for home/remote times
  - Alternate portrait mode: Note font pre-loaded for remote cities loop

### Added - Temperature Color Coding and Enhanced Environmental Display

#### Temperature Visualization
- **Color-coded temperature display** based on user-definable ranges:
  - **Blue**: Freezing (≤ 0°C)
  - **Cyan**: Cold (1-15°C)
  - **Green**: Pleasant (16-25°C)
  - **Orange**: Hot (26-35°C)
  - **Red**: Extreme (> 35°C)
- **Customizable thresholds**: Edit temperature ranges in src/main.cpp
- **Celsius-based color determination**: Consistent color coding regardless of display unit
- **Negative temperature support**: Correctly displays sub-zero temperatures with "-" prefix

#### Enhanced Environmental Data Display
- **Unit symbols added**:
  - Temperature: "oC" or "oF" (lowercase 'o' as degree symbol)
  - Pressure: "hPa" suffix
  - Humidity: Already had "%" symbol
- **Improved formatting**:
  - Clear separation of value and unit
  - Proper handling of negative values
  - Color-coded temperature in both landscape and alternate portrait modes

#### Alternate Portrait Mode Refinements
- **Updated day indicators**: Changed from "PREV DAY"/"NEXT DAY" to "Prev Day"/"Next Day" for consistency with standard portrait
- **Improved positioning**: Day indicators now directly below city name (no indent)
- **Cleaner layout**: Removed separator lines between remote cities
- **Better spacing**: Sensor data moved down to avoid overlap with home time
- **Optimized vertical space**: Remote cities use full available height

---

## [2.7.0] - 2026-01-30

### Added - Portrait Mode Environmental Display with Screen Rotation

#### Alternate Portrait Screen
- **Dual-screen rotation system** in portrait mode with environmental sensor
- **Analogue clock display** in portrait mode alternate screen
  - 55px radius clock face with 12 hour markers
  - Animated hour, minute, and second hands
  - Center dot with smooth hand movement
  - Matches landscape mode analogue clock styling
- **Home city focus layout**:
  - Large analogue clock (y=55-165)
  - Home city name with HOME indicator
  - Large digital time display
  - Environmental data (temperature, humidity, pressure)
- **Compact remote city list**:
  - 5 cities in 2-line format (17px per city)
  - City name + time on first line
  - PREV DAY / NEXT DAY indicator on second line
  - Efficient use of remaining space (y=235-320)

#### Screen Rotation Control
- **Configurable auto-flip** between standard and alternate portrait screens
- **Enable/disable toggle** in WebUI System Status section
- **Adjustable interval**: 3-30 seconds (default: 8 seconds)
- **Conditions for activation**:
  - Portrait mode (not landscape)
  - Environmental sensor available
  - Screen rotation enabled
- **Smooth transitions**: Preserves state between screen flips
- **Persistent settings**: Stored in NVS across reboots

#### WebUI Integration
- **New controls** in System Status section:
  - "Enable Screen Rotation (Portrait+Sensor)" checkbox
  - "Interval" number input (3-30 second range with validation)
- **Live mirror support**: Canvas shows both portrait layouts
  - Standard portrait: 6-city stacked list
  - Alternate portrait: Analogue clock + compact list
- **Real-time rendering**: Mirrors current TFT screen state
- **Automatic layout switching**: Based on `showingAlternateScreen` flag

#### API Enhancements
- **`/api/state` additions**:
  - `enableScreenRotation` - boolean flag
  - `screenFlipInterval` - seconds between flips (3-30)
  - `showingAlternateScreen` - current screen state
- **`/api/config` handlers**:
  - POST `enableScreenRotation` to toggle feature
  - POST `screenFlipInterval` with range validation
- **`/api/mirror` additions**:
  - `showingAlternateScreen` flag for WebUI rendering

### Changed
- **Config structure**: Added `enableScreenRotation` (bool), `screenFlipInterval` (uint8_t)
- **NVS keys**: Added `PREF_SCREEN_ROTATION`, `PREF_FLIP_INTERVAL`
- **Main loop**: Screen flip logic with configurable interval
- **Drawing functions**: New `drawAlternatePortraitStatic()` and `drawAlternatePortraitUpdate()`
- **WebUI JavaScript**: New rendering function `renderAlternatePortrait()`

### Technical Details
- **Memory usage**: ~300 lines added to main.cpp, ~200 lines to WebUI
- **Screen dimensions**: 240x320 (portrait)
- **Flip mechanism**: Time-based with `millis()` tracking
- **State preservation**: All cached state variables maintained across flips
- **Font consistency**: Uses same smooth fonts as standard portrait mode
- **Environmental format**: Matches landscape mode display format

### Layout Specifications

**Alternate Portrait Screen (240x320)**:
- Header: Title + Date (y=0-40)
- Analogue clock: Center (120, 110), Radius 55px (y=40-170)
- Home section: Name + HOME + Time + Environmental (y=172-235)
- Remote cities: 5 × 17px compact rows (y=235-320)

**Compact Remote City Format**:
- Line 1 (y+2): City name (left, 9px) + Time (right, 14px)
- Line 2 (y+14): PREV DAY / NEXT DAY indicator (8px, indented)

### User Experience
- **Non-intrusive**: Standard portrait mode unaffected when rotation disabled
- **Informative**: Both city times and environmental data visible
- **Customizable**: User controls enable state and flip speed
- **Intuitive**: Clear visual distinction between standard and alternate screens
- **Efficient**: Minimal redraw overhead with state caching

---

## [2.6.1] - 2026-01-30

### Added - Environmental Sensor Support & WebUI Mirror Integration

#### Multi-Sensor Support
- **I2C sensor support** for environmental monitoring on GPIO 22 (SCL) and GPIO 27 (SDA)
- **Four sensor types supported**:
  - **BMP280**: Temperature + Pressure (Bosch)
  - **BME280**: Temperature + Humidity + Pressure (Bosch)
  - **SHT3X**: Temperature + Humidity (Sensirion)
  - **HTU21D**: Temperature + Humidity (TE Connectivity)
- **Auto-detection**: Sensors are automatically detected at boot via I2C scan
- **Conditional compilation**: Select sensor type via `#define` in `include/config.h`
- **Graceful degradation**: Shows "N/A" when no sensor detected

#### Temperature Unit Selection
- **Celsius/Fahrenheit toggle** in WebUI with real-time conversion
- **Persistent storage**: Temperature unit preference saved in NVS
- **API support**: `useFahrenheit` boolean in `/api/state` and `/api/config`
- **Display formatting**: Shows temperature with appropriate unit symbol (°C or °F)

#### TFT Display Integration (Landscape Mode)
- **Environmental data display** below digital time in landscape mode left panel
- **Position**: y=218 (below digital time at y=181)
- **Adaptive format**:
  - BME280: `25° 65% 1013hPa` (temp, humidity, pressure)
  - BMP280: `25°  1013hPa` (temp, pressure)
  - SHT3X/HTU21D: `25°  65%` (temp, humidity)
- **Smooth font**: Uses `kFontNote` for consistent styling
- **Auto-hiding**: Only shows when in landscape mode and sensor available

#### WebUI Display Mirror Integration (New in 2.6.1)
- **Environmental data in mirror**: Canvas display now shows sensor readings matching TFT
- **Live updates**: Environmental data updates every 2 seconds with mirror refresh
- **Landscape mode only**: Displays at y=218 below digital time, just like TFT
- **Matching format**: Exact same format as TFT display (temp, humidity, pressure based on sensor)
- **API enhancement**: `/api/mirror` endpoint now includes:
  - `sensorAvailable` - boolean indicating sensor presence
  - `sensorType` - sensor model name
  - `envData` - pre-formatted environmental string
- **Color consistency**: Light grey (#C0C0C0) matching TFT_LIGHTGREY

#### WebUI Status Display
- **Sensor Type**: Shows detected sensor model (BMP280, BME280, SHT3X, HTU21D, or N/A)
- **Temperature**: Real-time temperature with unit toggle
- **Humidity**: Displays relative humidity percentage (when available)
- **Pressure**: Shows barometric pressure in hPa (when available)
- **Status updates**: Refreshes every 5 seconds

#### Serial Output
- **Periodic logging**: Sensor readings output to serial every 10 seconds
- **Debug formatting**: Shows sensor type and available measurements
- **Example**: `Sensor: BMP280 - 24.5°C, 1013.2 hPa`

### Changed
- **Config structure**: Added `bool useFahrenheit` field
- **NVS keys**: Added `PREF_FAHRENHEIT` for temperature unit storage
- **Library dependencies**: Added Adafruit sensor libraries (BMP280, BME280, SHT31, HTU21DF, Unified Sensor)
- **API endpoints**:
  - Updated `/api/state` to include sensor data and temperature unit
  - Updated `/api/mirror` to include environmental data
- **Project structure**: Added `include/config.h` for hardware configuration
- **WebUI JavaScript**: Updated `app.js` with environmental data rendering in landscape mirror

### Technical Details
- **I2C pins**: SDA=GPIO27, SCL=GPIO22 (CYD Temp/Humidity Interface)
- **Sensor polling**: 10-second update interval
- **Memory impact**: Minimal - sensor objects conditionally compiled
- **Initialization**: `testSensor()` auto-detects and initializes appropriate driver
- **Data reading**: `updateSensorData()` supports all sensor types with conditional compilation
- **Mirror format**: Pre-formatted string sent via API reduces WebUI complexity

---

## [2.6.0] - 2026-01-28

### Added - Flip Display & Screenshot Improvements

#### Flip Display Feature
- **180° rotation option** allows mounting CYD with USB connector on either side
- **Portrait mode**: USB bottom (normal) or top (flipped)
- **Landscape mode**: USB right (normal) or left (flipped)
- **Persistent storage**: Flip setting saved in NVS, survives reboots
- **WebUI control**: Checkbox next to Display Mode dropdown
- **API support**: `flipDisplay` boolean in `/api/state` and `/api/config` endpoints
- **Rotation values**: 0=portrait, 1=landscape, 2=portrait-flipped, 3=landscape-flipped

#### Screenshot Capture Improvements
- **Changed to BMP format** from JPEG for better reliability
- **Removed JPEGENC library dependency** - simpler codebase
- **Row-by-row streaming** minimizes memory usage (~1KB buffer vs 150KB+)
- **File format**: 24-bit BMP with proper padding and bottom-up row order
- **File size**: ~230KB uncompressed (trade-off for reliability)
- **Downloads as**: `clock_snapshot.bmp`

### Fixed

#### Colon Blinking on Startup
- **Problem**: After firmware upload, colons didn't blink until config change or mode switch
- **Root cause**: `lastColonState` initialization timing issue with first draw
- **Solution**:
  - Simplified `lastColonState` initialization to `true`
  - First draw triggers via `timeChanged=true` (empty string comparison)
  - Subsequent draws handle colon blink correctly
- **Result**: Colons blink immediately on boot

#### Text Padding in Landscape Mode
- **Problem**: Home city time (left panel) showed ghosting/artifacts when colons blinked
- **Root cause**: Missing `setTextPadding()` call for home city and remote city times
- **Solution**: Added `tft.setTextPadding(timePadWidth)` to both:
  - Home city digital time (below analog clock)
  - Remote cities time display (right panel)
- **Result**: Clean text updates with no artifacts

### Changed
- **Config structure**: Added `bool flipDisplay` field
- **NVS keys**: Added `PREF_FLIP` for persistent flip storage
- **Rotation logic**: `applyRotation()` now calculates rotation 0-3 based on landscape+flip
- **Library dependencies**: Removed `bitbank2/JPEGENC @ ^1.0.1`
- **WebUI**: Added flip display checkbox in System Status section

### Documentation
- **CLAUDE.md**: Updated with flip display feature and BMP screenshot details
- **platformio.ini**: Removed JPEGENC library dependency

---

## [2.5.0] - 2026-01-28

### Added - NEXT DAY Indicator & Screenshot Capture

#### NEXT DAY Indicator
- **New visual indicator** for cities ahead of home city (e.g., home=London, remote=Sydney shows "NEXT DAY")
- **Cyan color** (#00FFFF) to distinguish from yellow "PREV DAY" indicator
- **Works in both modes**: Portrait and Landscape display modes
- **WebUI mirror support**: Canvas renderer shows NEXT DAY indicator
- **API updated**: `/api/mirror` now includes `nextDay` boolean for each city

#### Screenshot Capture via WebUI
- **New endpoint**: `GET /api/snapshot` streams BMP image directly from TFT display
- **True pixel capture**: Downloads actual TFT framebuffer data, not canvas recreation
- **BMP format**: 24-bit RGB, row-by-row streaming to minimize memory usage
- **Colon sync**: Waits for even second to ensure colons are visible in screenshot
- **WebUI button**: "Capture Screenshot" button in Display Mirror section
- **Filename**: Downloads as `clock_snapshot.bmp`

### Fixed

#### Colon Alignment Issue
- **Problem**: The ":" between HH and MM was misaligned initially, fixing itself after a minute
- **Root cause**: Selective colon redraw was drawing at slightly different position than full time string
- **Solution**: Changed from selective colon update to full time string redraw
  - When colon visible: draws "HH:MM"
  - When colon hidden: draws "HH MM" (space instead of colon)
- **Applied to**: Portrait mode, landscape home city, and landscape remote cities

#### PREV/NEXT DAY Update Delay
- **Problem**: After config change (e.g., swapping Sydney↔London), PREV/NEXT DAY took several seconds to update
- **Solution**: Added `lastBatchUpdate = 0;` after config save to force immediate time recalculation

### Changed
- **WebUI**: Added snapshot button with loading state and success notification
- **API response**: `/api/mirror` includes `nextDay` field for all cities
- **README**: Updated with TFT display screenshots (TFT_Portrait.jpg, TFT_Landscape.jpg) above WebUI screenshots

### Documentation
- **README.md**: Added TFT Display and Web Interface screenshot sections
- **Visual Indicators**: Updated to document both PREV DAY (yellow) and NEXT DAY (cyan)

---

## [2.4.0] - 2026-01-28

### Added - Landscape Mode with Analogue Clock

#### Dual Display Mode Support

- **Portrait Mode (240x320)**: Original vertical layout with all 6 cities stacked
- **Landscape Mode (320x240)**: New horizontal layout with analogue clock
  - Left panel (120px): Title, date, analogue clock, home city name, HOME indicator, digital time
  - Right panel (200px): 5 remote cities with times and PREV DAY indicators
- **Persistent Mode Selection**: Display mode saved in NVS, survives reboots
- **WebUI Toggle**: Switch between modes via dropdown in System Status section

#### Analogue Clock (Landscape Mode)

- **Real-time clock face** with hour markers at 12, 3, 6, 9 positions
- **Three hands**:
  - Hour hand (white, thick) - moves smoothly with minutes
  - Minute hand (white, medium) - updates every minute
  - Second hand (red, thin) - updates every second
- **Flicker-free animation**: Selective redraw erases old hand positions before drawing new
- **Clock dimensions**: 50px radius, centered in left panel at (60, 95)

#### WebUI Live Clock Display

- **Real-time mirror** of all city times in the web interface
- **2-second polling** for responsive display updates
- **Visual elements**:
  - Home city with cyan "Home" indicator
  - Remote cities with yellow "Prev Day" indicator when applicable
  - Large monospace font for easy reading
- **New API endpoint**: `GET /api/mirror` returns current clock display data

#### LDR (Light Dependent Resistor) Support

- **Ambient light sensing** on GPIO34
- **10-sample averaging** for noise reduction
- **LDR value displayed** in WebUI System Status section
- **ADC range**: 0-4095 (12-bit)

### Changed

#### Layout Constants

- Added landscape-specific layout constants:
  - `kLeftPanelWidth = 120` - Left panel for clock and home city
  - `kRightPanelWidth = 200` - Right panel for remote cities
  - `kLandscapeRemoteRowHeight = 48` - Height per remote city row
- Added analogue clock constants:
  - `kClockCenterX = 60`, `kClockCenterY = 95` - Clock face center
  - `kClockRadius = 50` - Clock face radius
  - Hand lengths: hour (25), minute (35), second (40)
  - Colors: face (dark grey), markers (white), hands (white/white/red)

#### WebUI Enhancements

- **Polling interval** reduced from 5 seconds to 2 seconds for live clock display
- **Display mode selector** added to System Status section
- **LDR value** displayed in status grid
- **Status hint** updated to indicate touch diagnostics and 5-second refresh

#### API Changes

- `GET /api/state` now includes `landscapeMode` and `ldrValue` fields
- `POST /api/config` accepts `landscapeMode` boolean to change display orientation
- New `GET /api/mirror` endpoint for live clock data

### Technical Details

#### New Functions

- `drawAnalogClockFace()` - Draws static clock face with hour markers
- `updateAnalogClockHands()` - Updates hands with selective erase/redraw
- `drawClockHand()` - Draws single hand with configurable thickness
- `drawStaticLayoutLandscape()` - Draws landscape mode static elements
- `drawTimesLandscape()` - Updates times in landscape mode
- `readLDR()` - Reads and averages LDR sensor value
- `applyRotation()` - Sets TFT and touch rotation based on config

#### Analogue Clock State

- `lastSecond`, `lastMinute`, `lastHour` - Track previous hand positions for selective erase
- State reset on config change, diagnostics exit, and mode switch

#### Memory Impact

- **Flash**: ~1,050KB (increased ~35KB for landscape/clock code)
- **RAM**: ~54KB (increased ~1KB for clock state variables)

---

## [2.3.1] - 2026-01-26

### Added
- **Mobile responsive design** for WebUI
  - Automatic font scaling for mobile devices (screens ≤600px)
  - Optimized layout for phone and tablet viewing
  - Reduced font sizes and spacing for smaller screens
- **WiFiManager auto-request handlers** to suppress browser connectivity check errors
  - Handles `/favicon.ico`, `/generate_204` (Android), `/hotspot-detect.html` (iOS)
  - Eliminates `[E][WebServer.cpp:649] _handleRequest(): request handler not found` errors

### Changed
- **WiFi configuration portal timeout** - Changed from 180 seconds to indefinite
  - Portal stays open until WiFi is configured
  - Prevents timeout during slow configuration
  - Updated on-screen message: "Portal stays open until configured"
- **WiFi AP naming** - Simplified from "CYD-WorldClock-*" to "WorldClock-*"
  - Setup AP: `WorldClock-Setup` (was `CYD-WorldClock-Setup`)
  - Fallback AP: `WorldClock-AP` (was `CYD-WorldClock-AP`)
  - OTA hostname: `WorldClock` (was `CYD-WorldClock`)
- **WebUI clock display enhancements**
  - Increased font sizes for better readability on desktop
  - City names: 2rem, Times: 2.6rem
  - Home/PREV DAY labels: 1rem (50% smaller for cleaner look)
  - Improved spacing and alignment
  - Reduced vertical spacing between city names and labels

### Fixed
- WebUI font size consistency across all cities
- Mobile view now properly scales all elements
- WiFiManager captive portal browser error messages

## [2.3.0] - 2026-01-24

### Fixed - Complete Memory Leak Elimination

#### Problem
- v2.2.0 reduced but did not eliminate the setenv() memory leak
- Device still leaked ~3 bytes/second (~11KB/hour) due to 6 setenv() calls per minute
- Over extended periods, heap would still degrade

#### Solution - Manual Timezone Calculation
- **Eliminated ALL setenv() calls** from the main loop
- Implemented manual POSIX timezone string parsing
- Parse TZ strings once at config load, not every time calculation
- Calculate local time using UTC + offset instead of setenv()/localtime_r()

#### New Components
- `ParsedTimezone` struct stores parsed TZ data (offsets, DST rules)
- `parseTimezoneString()` parses POSIX TZ format (e.g., "AEST-10AEDT,M10.1.0/2,M4.1.0/3")
- `isDstActive()` calculates DST transitions using M.w.d rules
- `getLocalTimeNoSetenv()` converts UTC to local time without setenv()
- Handles both northern and southern hemisphere DST patterns

#### Results
- **Zero setenv() calls in main loop = Zero memory leak**
- Heap completely stable over indefinite runtime
- Slightly increased flash (+24KB for TZ parser) and RAM (+2.7KB for parsed data)
- No functional changes to user experience

### Removed
- `getLocalTm()` function (used setenv internally)
- `lastSetTimezone` caching variable (no longer needed)

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
- **Fallback AP mode** when WiFiManager fails (WorldClock-AP)
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

### v2.5.0 (Planned)

- [ ] WiFi reconnect logic in main loop
- [ ] Automatic NTP resync every 24 hours
- [ ] Automatic brightness control using LDR
- [ ] Touch-based city selection/editing

### v3.0.0 (Future)

- [ ] Weather API integration per city
- [ ] Temperature/humidity display
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
