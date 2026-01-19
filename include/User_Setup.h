// TFT_eSPI User Setup for ESP32-2432S028 (CYD 2.8" ILI9341)
// If your module differs (ESP32-2432S024 / ST7789 etc.), update driver + pins here.
//
// PlatformIO uses this via:
//   -DUSER_SETUP_LOADED -include include/User_Setup.h

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// SPI pins (common CYD defaults)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1  // Connect TFT RST to ESP32 RST (common CYD), else set to a GPIO

// Backlight control (if present on your CYD)
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Touch panel configuration for CYD (XPT2046)
#define TOUCH_CS 33

// Enable touch screen support
#define SUPPORT_TOUCH
#define XPT2046_DRIVER

// Touch screen calibration (adjust if needed for your display)
#define TOUCH_CALIBRATION_X1 200
#define TOUCH_CALIBRATION_X2 3700
#define TOUCH_CALIBRATION_Y1 200
#define TOUCH_CALIBRATION_Y2 3700

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
