# CYD World Clock

With Family all around the Globe I wanted to not have to open Apps on phone, clocks on computers and have a small little display showing the time at various places around the world, so this was created.

A feature-rich world clock display for the ESP32 CYD (Cheap Yellow Display) featuring a 2.8" ILI9341 TFT screen. Display multiple timezones simultaneously with a beautiful interface, web-based configuration, and OTA updates.

![Version](https://img.shields.io/badge/version-2.0.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![Framework](https://img.shields.io/badge/framework-Arduino-orange)

## Features

### Display
- **5 Timezone Display**: Configurable home city + 4 remote cities
- **Centered Layout**: Title, date, and city times in clean rows
- **Smooth Fonts**: Optional TFT_eSPI smooth fonts from LittleFS
- **Visual Indicators**:
  - Blinking colon every second
  - "PREV DAY" yellow indicator for cities in previous day
  - Color-coded status messages

### Configuration
- **Web-Based Config**: Configure all cities via browser interface
- **NVS Storage**: Persistent timezone configuration across reboots
- **WiFiManager**: Easy WiFi setup with captive portal
- **Default Cities**: Sydney, Vancouver, London, Nairobi, Denver

### Connectivity
- **WiFi**: Auto-connect with fallback AP mode
- **OTA Updates**: Wireless firmware updates with progress bar
- **Web Server**: REST API and static file serving
- **NTP Sync**: Automatic time synchronization on boot

### Developer Features
- **5-Level Debug System**: Runtime-adjustable logging (Off/Error/Warn/Info/Verbose)
- **Startup Display**: Boot messages shown on screen
- **Splash Screen**: Globe animation on startup
- **State Caching**: Minimal redraws for flicker-free updates

## Hardware Requirements

- **Board**: ESP32 CYD (ESP32-2432S028)
- **Display**: ILI9341 2.8" TFT (240x320 pixels)
- **Power**: USB 5V

### Pin Configuration

| Function | GPIO | Notes |
|----------|------|-------|
| TFT_MOSI | 13 | SPI data out |
| TFT_MISO | 12 | SPI data in |
| TFT_SCLK | 14 | SPI clock |
| TFT_CS   | 15 | Chip select |
| TFT_DC   | 2  | Data/command |
| TFT_RST  | -1 | Tied to ESP32 RST |
| TFT_BL   | 21 | Backlight (active HIGH) |

## Installation

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- ESP32 CYD board
- USB cable for programming

### Build & Upload

1. **Clone or download this repository**

2. **Build the firmware:**
   ```bash
   pio run
   ```

3. **Upload firmware to ESP32:**
   ```bash
   pio run -t upload
   ```

4. **Upload web UI files to LittleFS:**
   ```bash
   pio run -t uploadfs
   ```

5. **Monitor serial output (optional):**
   ```bash
   pio device monitor
   ```

### First-Time Setup

1. **WiFi Configuration:**
   - On first boot, device creates AP: `CYD-WorldClock-Setup`
   - Connect to this AP with your phone/laptop
   - Enter your WiFi credentials in the captive portal
   - Device will connect and display IP address on screen

2. **Access Web Interface:**
   - Note the IP address shown on the display
   - Open browser to `http://<device-ip>`
   - Configure your home city and 4 remote cities
   - Click "Save Configuration"
   - Reboot device to apply changes

## Web Interface

The web UI provides:

- **System Status**: Firmware version, uptime, WiFi info
- **Timezone Configuration**:
  - Home city (reference timezone)
  - 4 remote cities
  - POSIX timezone string inputs
- **Common Timezone Reference**: Quick copy for popular cities
- **WiFi Reset**: Clear credentials and restart

### API Endpoints

- `GET /api/state` - Returns system status and current configuration (JSON)
- `POST /api/config` - Update timezone configuration (JSON body)
- `POST /api/reset-wifi` - Clear WiFi credentials and reboot

## Configuration

### Timezone Strings (POSIX Format)

The clock uses POSIX timezone strings. Common examples:

| City | Timezone String |
|------|----------------|
| Sydney | `AEST-10AEDT,M10.1.0/2,M4.1.0/3` |
| Vancouver | `PST8PDT,M3.2.0/2,M11.1.0/2` |
| London | `GMT0BST,M3.5.0/1,M10.5.0/2` |
| New York | `EST5EDT,M3.2.0/2,M11.1.0/2` |
| Tokyo | `JST-9` |
| Singapore | `SGT-8` |
| Dubai | `GST-4` |
| Paris | `CET-1CEST,M3.5.0,M10.5.0/3` |

Format: `STD offset DST,start_rule,end_rule`
- `STD`: Standard time name
- `offset`: Hours offset from UTC (note: sign is reversed)
- `DST`: Daylight saving time name
- `start_rule`: When DST starts (Month.Week.Day/Hour)
- `end_rule`: When DST ends

### Customization

Edit `src/main.cpp` to customize:

- **Colors** (lines 138-141): `COLOR_BG`, `COLOR_LABEL`, `COLOR_TIME`
- **Layout** (lines 144-147): Header heights, padding
- **Fonts** (lines 150-157): Font names and fallback sizes
- **Default Cities** (lines 83-91): Initial configuration
- **Debug Level** (line 36): Default logging verbosity (0-4)
- **OTA Password** (line 70): ⚠️ **Change from default "change-me"!**

## OTA Updates

### Configuration
- **Hostname**: `CYD-WorldClock`
- **Password**: `change-me` (⚠️ **Change this in production!**)

### Using Arduino IDE
1. Tools → Port → Select network port `CYD-WorldClock at <ip>`
2. Upload as normal

### Using PlatformIO
```bash
pio run -t upload --upload-port <device-ip>
```

Progress bar is displayed on the TFT during updates.

## Debug System

### Debug Levels
- **0 (Off)**: No debug output
- **1 (Error)**: Critical errors only
- **2 (Warn)**: Warnings + errors
- **3 (Info)**: General information (default)
- **4 (Verbose)**: All debug output

### Serial Monitor
Baud rate: **115200**

Example output:
```
[INFO] CYD World Clock v2.0.0 starting...
[INFO] ✓ LittleFS mounted
[INFO] Config loaded: Home=SYDNEY, Remote0=VANCOUVER
[INFO] WiFi connected: SSID=MyNetwork IP=192.168.1.100
[INFO] ✓ OTA ready
[INFO] ✓ Web server started on port 80
[INFO] NTP synced to AEST-10AEDT,M10.1.0/2,M4.1.0/3
```

## Troubleshooting

### Display Issues
- **Blank screen**: Check backlight pin (GPIO 21), verify TFT_eSPI config
- **Text cutoff**: Verify font files uploaded to LittleFS
- **Wrong colors**: Check `include/User_Setup.h` for correct driver

### WiFi Issues
- **Can't connect**: Use web UI to reset WiFi credentials
- **AP not visible**: Power cycle device, wait 30 seconds
- **Connection timeout**: Check WiFi signal strength, verify credentials

### Web Interface Issues
- **Can't access**: Verify IP address on display matches URL
- **Changes not saved**: Check browser console for errors
- **Need to reboot**: After config changes, reboot device to apply

### NTP Sync Issues
- **Time incorrect**: Check WiFi connection, verify timezone string
- **Sync fails**: Firewall may be blocking NTP (port 123 UDP)

## File Structure

```
CYD_Family_Clock/
├── src/
│   └── main.cpp              # Main application code (~760 lines)
├── include/
│   └── User_Setup.h          # TFT_eSPI hardware configuration
├── data/                     # LittleFS files (upload with uploadfs)
│   ├── index.html            # Web UI interface
│   ├── app.js                # Web UI JavaScript
│   ├── style.css             # Web UI styling
│   ├── NotoSans-Bold7.vlw    # Smooth fonts (optional)
│   ├── NotoSans-Bold9.vlw
│   ├── NotoSans-Bold10.vlw
│   └── NotoSans-Bold16.vlw
├── platformio.ini            # PlatformIO configuration
├── CLAUDE.md                 # Project documentation
├── README.md                 # This file
└── CHANGELOG.md              # Version history
```

## Memory Usage

- **Flash**: 1,010,529 bytes (77.1% of 1.3MB)
- **RAM**: 51,024 bytes (15.6% of 320KB)

## Dependencies

- **TFT_eSPI** @ ^2.5.43 - Display driver
- **WiFiManager** @ ^2.0.16-rc.2 - WiFi configuration
- **ArduinoJson** @ ^6.21.3 - JSON parsing (minimal usage)
- **ArduinoOTA** @ 2.0.0 - OTA updates
- **WebServer** @ 2.0.0 - HTTP server
- **Preferences** @ 2.0.0 - NVS storage
- **LittleFS** @ 2.0.0 - Filesystem

## Contributing

This is a personal project, but suggestions and bug reports are welcome!

## License

This project is provided as-is for personal use.

## Credits

- **Architecture inspired by**: [ESP32_Touchdown_Retro_Clock](https://github.com/anthonyjclarke)
- **TFT_eSPI Library**: Bodmer
- **WiFiManager**: tzapu

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.

## Author

**Anthony Clarke**
- GitHub: [@anthonyjclarke](https://github.com/anthonyjclarke)

---

**Built with ESP32 • Displayed on ILI9341 • Powered by Arduino**
