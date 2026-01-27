// CYD World Clock: ESP32 + ILI9341 world clock display with WiFiManager + NTP.
#include <Arduino.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <time.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "timezones.h"

// =========================
// Debug System
// =========================
/**
 * Leveled debug logging system with runtime control
 *
 * DEBUG LEVELS:
 *   0 = Off      - No debug output
 *   1 = Error    - Critical errors only
 *   2 = Warn     - Warnings + Errors
 *   3 = Info     - General info + Warnings + Errors (default)
 *   4 = Verbose  - All debug output including frequent events
 *
 * USAGE:
 *   DBG_ERROR(...)   - Critical errors (level 1+)
 *   DBG_WARN(...)    - Warnings (level 2+)
 *   DBG_INFO(...)    - General information (level 3+)
 *   DBG_INFO(...) - Verbose/frequent output (level 4)
 *
 * RUNTIME CONTROL:
 *   Set debugLevel variable (0-4) to change verbosity at runtime
 *   Can be controlled via web API
 */
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 3  // Default: Info level
#endif

#define DBG_LEVEL_OFF     0
#define DBG_LEVEL_ERROR   1
#define DBG_LEVEL_WARN    2
#define DBG_LEVEL_INFO    3
#define DBG_LEVEL_VERBOSE 4

// Runtime debug level control (can be changed via web API)
static uint8_t debugLevel = DEBUG_LEVEL;

// Forward declaration for log buffer
void addToLogBuffer(uint8_t level, const char* msg);

// Helper to get current timestamp for debug output (in home timezone)
String getDebugTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  // Use localtime_r to get time in system timezone (set to home city)
  localtime_r(&now, &timeinfo);
  char buf[24];
  snprintf(buf, sizeof(buf), "[%02d-%02d-%02d : %02d:%02d:%02d]",
           timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buf);
}

// Helper to format and add to log buffer
void logToBuffer(uint8_t level, const char* format, ...) {
  char buffer[80];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  addToLogBuffer(level, buffer);
}

// Conditional debug macros based on debug level - now also log to buffer with timestamps
// Buffer size matches log buffer entry size (80 bytes) to avoid memory waste
#define DBG_ERROR(...)   do { if (debugLevel >= DBG_LEVEL_ERROR) { Serial.print("[ERR ] "); Serial.print(getDebugTimestamp()); Serial.print(" "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_ERROR, buf); } } while(0)
#define DBG_WARN(...)    do { if (debugLevel >= DBG_LEVEL_WARN) { Serial.print("[WARN] "); Serial.print(getDebugTimestamp()); Serial.print(" "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_WARN, buf); } } while(0)
#define DBG_INFO(...)    do { if (debugLevel >= DBG_LEVEL_INFO) { Serial.print("[INFO] "); Serial.print(getDebugTimestamp()); Serial.print(" "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_INFO, buf); } } while(0)
#define DBG_VERBOSE(...) do { if (debugLevel >= DBG_LEVEL_VERBOSE) { Serial.print("[VERB] "); Serial.print(getDebugTimestamp()); Serial.print(" "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_VERBOSE, buf); } } while(0)

// Legacy compatibility macros
#define DBG(...)      DBG_INFO(__VA_ARGS__)
#define DBGLN(s)      DBG_INFO("%s\n", s)
#define DBG_STEP(s)   DBG_INFO("%s\n", s)
#define DBG_OK(s)     DBG_INFO("✓ %s\n", s)
#define DBG_ERR(s)    DBG_ERROR("%s\n", s)

// =========================
// Hardware Pin Definitions
// =========================
// Touch screen pins (XPT2046) - confirmed working from fluid simulation demo
// Note: IRQ and MISO swapped from some CYD documentation
#define XPT2046_IRQ  36   // T_IRQ (interrupt, active LOW when touched)
#define XPT2046_MOSI 32   // T_DIN
#define XPT2046_MISO 39   // T_OUT
#define XPT2046_CLK  25   // T_CLK
#define XPT2046_CS   33   // T_CS

// LDR (Light Dependent Resistor)
#define LDR_PIN      34   // Analog input for LDR

// Touch calibration values (from fluid simulation)
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800

// =========================
// Global Objects & Configuration
// =========================
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Preferences prefs;
SPIClass touchSPI = SPIClass(VSPI);  // Separate SPI bus for touch
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Cached WiFi info (updated on connect, prevents String allocation every API call)
char cachedSSID[33] = "";
char cachedIP[16] = "";
int cachedRSSI = 0;

// =========================
// Log Buffer System
// =========================
#define LOG_BUFFER_SIZE 20

struct LogEntry {
  unsigned long timestamp;  // millis() when logged
  uint8_t level;           // DBG_LEVEL_ERROR, WARN, INFO, VERBOSE
  char message[80];        // Truncated message
};

LogEntry logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;
int logCount = 0;

// Add entry to circular log buffer
void addToLogBuffer(uint8_t level, const char* msg) {
  logBuffer[logIndex].timestamp = millis();
  logBuffer[logIndex].level = level;
  strlcpy(logBuffer[logIndex].message, msg, sizeof(logBuffer[logIndex].message));
  
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) {
    logCount++;
  }
}

// =========================
// Diagnostics Screen State
// =========================
bool showingDiagnostics = false;
unsigned long diagnosticsStartTime = 0;
const unsigned long DIAGNOSTICS_TIMEOUT = 15000; // 15 seconds

#define FIRMWARE_VERSION "2.5.0"
#define OTA_HOSTNAME "WorldClock"
#define OTA_PASSWORD "change-me"  // TODO: Change this!

// Configuration structure
struct Config {
  char homeCityLabel[32];
  char homeCityTz[64];
  char remoteCities[5][32];    // City labels (expanded to 5)
  char remoteTzStrings[5][64]; // Timezone strings (expanded to 5)
  bool landscapeMode;          // Display orientation: true = landscape, false = portrait
};

Config config;

// =========================
// Manual Timezone Calculation (replaces setenv() to fix memory leak)
// =========================
// POSIX TZ format: STD offset [DST [offset], start [/time], end [/time]]
// Example: "AEST-10AEDT,M10.1.0/2,M4.1.0/3"
// Note: POSIX sign is inverted - negative offset means AHEAD of UTC

struct DstRule {
  uint8_t month;    // 1-12
  uint8_t week;     // 1-5 (5 = last)
  uint8_t dow;      // 0-6 (0 = Sunday)
  uint8_t hour;     // Transition hour (default 2)
};

struct ParsedTimezone {
  int16_t stdOffsetMins;   // Standard time offset in minutes from UTC
  int16_t dstOffsetMins;   // DST offset in minutes from UTC (0 if no DST)
  bool hasDst;
  DstRule dstStart;        // When DST begins
  DstRule dstEnd;          // When DST ends
};

// Parsed timezone cache for all 6 cities
static ParsedTimezone parsedTz[6];

// Parse hours:minutes offset string, returns minutes
// Handles formats: "10", "-5", "5:30", "-9:30"
static int16_t parseOffset(const char* str, const char** endPtr) {
  int sign = 1;
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }

  int hours = 0;
  while (*str >= '0' && *str <= '9') {
    hours = hours * 10 + (*str - '0');
    str++;
  }

  int mins = 0;
  if (*str == ':') {
    str++;
    while (*str >= '0' && *str <= '9') {
      mins = mins * 10 + (*str - '0');
      str++;
    }
  }

  if (endPtr) *endPtr = str;
  // POSIX sign is inverted: -10 means UTC+10, so we negate
  return -sign * (hours * 60 + mins);
}

// Parse DST rule in M.w.d/h format
// M = month (1-12), w = week (1-5), d = day (0-6), h = hour
static bool parseDstRule(const char* str, DstRule* rule, const char** endPtr) {
  if (*str != 'M') return false;
  str++;

  rule->month = 0;
  while (*str >= '0' && *str <= '9') {
    rule->month = rule->month * 10 + (*str - '0');
    str++;
  }
  if (*str != '.') return false;
  str++;

  rule->week = *str - '0';
  str++;
  if (*str != '.') return false;
  str++;

  rule->dow = *str - '0';
  str++;

  // Optional hour (default 2:00)
  rule->hour = 2;
  if (*str == '/') {
    str++;
    rule->hour = 0;
    while (*str >= '0' && *str <= '9') {
      rule->hour = rule->hour * 10 + (*str - '0');
      str++;
    }
  }

  if (endPtr) *endPtr = str;
  return true;
}

// Parse full POSIX TZ string
void parseTimezoneString(const char* tzStr, ParsedTimezone* tz) {
  memset(tz, 0, sizeof(ParsedTimezone));

  const char* p = tzStr;

  // Skip STD name (alphabetic characters)
  while (*p && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) p++;

  // Parse STD offset
  tz->stdOffsetMins = parseOffset(p, &p);

  // Check for DST
  if (!*p || *p == '\0') {
    tz->hasDst = false;
    tz->dstOffsetMins = tz->stdOffsetMins;
    return;
  }

  // Skip DST name
  while (*p && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) p++;

  // DST offset (optional - defaults to STD + 60 mins)
  tz->hasDst = true;
  if (*p == ',' || *p == '\0') {
    tz->dstOffsetMins = tz->stdOffsetMins + 60;
  } else {
    tz->dstOffsetMins = parseOffset(p, &p);
  }

  // Parse DST start rule
  if (*p == ',') {
    p++;
    parseDstRule(p, &tz->dstStart, &p);
  }

  // Parse DST end rule
  if (*p == ',') {
    p++;
    parseDstRule(p, &tz->dstEnd, &p);
  }
}

// Calculate day of week for a given date (0 = Sunday)
// Using Zeller-like formula
static int dayOfWeek(int year, int month, int day) {
  if (month < 3) {
    month += 12;
    year--;
  }
  int k = year % 100;
  int j = year / 100;
  int dow = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
  // Adjust to 0=Sunday
  dow = (dow + 6) % 7;
  return dow;
}

// Get day of month for DST transition
// rule: M.w.d where w=1-4 means "nth occurrence", w=5 means "last"
static int getDstTransitionDay(int year, const DstRule* rule) {
  int month = rule->month;
  int targetDow = rule->dow;
  int week = rule->week;

  if (week == 5) {
    // Last occurrence: start from end of month
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // Leap year check
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
      daysInMonth[2] = 29;
    }
    int lastDay = daysInMonth[month];
    int lastDow = dayOfWeek(year, month, lastDay);
    int diff = lastDow - targetDow;
    if (diff < 0) diff += 7;
    return lastDay - diff;
  } else {
    // Nth occurrence: find first occurrence then add weeks
    int firstDow = dayOfWeek(year, month, 1);
    int diff = targetDow - firstDow;
    if (diff < 0) diff += 7;
    int firstOccurrence = 1 + diff;
    return firstOccurrence + (week - 1) * 7;
  }
}

// Check if DST is active for given UTC time
static bool isDstActive(time_t utc, const ParsedTimezone* tz) {
  if (!tz->hasDst) return false;

  // Convert to approximate local time for date calculations
  // Use standard offset as baseline
  time_t approxLocal = utc + (tz->stdOffsetMins * 60);
  struct tm ltm;
  gmtime_r(&approxLocal, &ltm);

  int year = ltm.tm_year + 1900;
  int month = ltm.tm_mon + 1;
  int day = ltm.tm_mday;
  int hour = ltm.tm_hour;

  // Get transition days for this year
  int startDay = getDstTransitionDay(year, &tz->dstStart);
  int endDay = getDstTransitionDay(year, &tz->dstEnd);

  int startMonth = tz->dstStart.month;
  int endMonth = tz->dstEnd.month;
  int startHour = tz->dstStart.hour;
  int endHour = tz->dstEnd.hour;

  // Southern hemisphere: DST spans year boundary (start > end month)
  bool southernHemisphere = (startMonth > endMonth);

  // Create comparable values: month * 10000 + day * 100 + hour
  int current = month * 10000 + day * 100 + hour;
  int start = startMonth * 10000 + startDay * 100 + startHour;
  int end = endMonth * 10000 + endDay * 100 + endHour;

  if (southernHemisphere) {
    // DST active from start (e.g., Oct) through end of year, and start of year through end (e.g., Apr)
    return (current >= start || current < end);
  } else {
    // Northern hemisphere: DST active between start and end
    return (current >= start && current < end);
  }
}

// Get local time from UTC without using setenv() - NO MEMORY LEAK
void getLocalTimeNoSetenv(time_t utc, const ParsedTimezone* tz, struct tm* out) {
  int16_t offsetMins = isDstActive(utc, tz) ? tz->dstOffsetMins : tz->stdOffsetMins;
  time_t local = utc + (offsetMins * 60);
  gmtime_r(&local, out);
}

// Parse all configured timezones (call after config load)
void parseAllTimezones() {
  parseTimezoneString(config.homeCityTz, &parsedTz[0]);
  for (int i = 0; i < 5; i++) {
    parseTimezoneString(config.remoteTzStrings[i], &parsedTz[i + 1]);
  }
  DBG_INFO("Parsed %d timezones (no setenv)\n", 6);
}

// Default configuration
const char *DEFAULT_HOME_LABEL = "SYDNEY";
const char *DEFAULT_HOME_TZ = "AEST-10AEDT,M10.1.0/2,M4.1.0/3";
const char *DEFAULT_REMOTE_LABELS[] = {"VANCOUVER", "LONDON", "NAIROBI", "DENVER", "TOKYO"};
const char *DEFAULT_REMOTE_TZS[] = {
    "PST8PDT,M3.2.0/2,M11.1.0/2",
    "GMT0BST,M3.5.0/1,M10.5.0/2",
    "EAT-3",
    "MST7MDT,M3.2.0/2,M11.1.0/2",
    "JST-9"
};

// =========================
// Configuration Storage (NVS)
// =========================
#define PREF_NAMESPACE "worldclock"
#define PREF_HOME_LABEL "homeLabel"
#define PREF_HOME_TZ "homeTz"
#define PREF_REMOTE_PREFIX "remote"  // remote0Label, remote0Tz, etc.
#define PREF_LANDSCAPE "landscape"   // Display orientation: true = landscape

// Helper: Extract city name only (strip country after comma)
// FIXED: Operates on char* instead of String to avoid heap allocation
void extractCityName(const char* fullLabel, char* outBuf, size_t bufSize) {
  const char* commaPos = strchr(fullLabel, ',');
  if (commaPos != nullptr && commaPos != fullLabel) {
    size_t copyLen = (size_t)(commaPos - fullLabel);
    if (copyLen >= bufSize) copyLen = bufSize - 1;
    strlcpy(outBuf, fullLabel, copyLen + 1);
  } else {
    strlcpy(outBuf, fullLabel, bufSize);
  }
}

// Load configuration from NVS
void loadConfig() {
  prefs.begin(PREF_NAMESPACE, false);

  // Load home city
  String homeLabel = prefs.getString(PREF_HOME_LABEL, DEFAULT_HOME_LABEL);
  String homeTz = prefs.getString(PREF_HOME_TZ, DEFAULT_HOME_TZ);
  DBG_INFO("NVS read homeLabel='%s'\n", homeLabel.c_str());
  // FIXED: Use char buffer instead of String to avoid heap allocation
  char homeCityBuf[32];
  extractCityName(homeLabel.c_str(), homeCityBuf, sizeof(homeCityBuf));
  DBG_INFO("After extract: homeCityOnly='%s'\n", homeCityBuf);
  strlcpy(config.homeCityLabel, homeCityBuf, sizeof(config.homeCityLabel));
  DBG_INFO("After strlcpy: config.homeCityLabel='%s'\n", config.homeCityLabel);
  strlcpy(config.homeCityTz, homeTz.c_str(), sizeof(config.homeCityTz));

  // Load 5 remote cities
  for (int i = 0; i < 5; i++) {
    // FIXED: Use char buffer instead of String to avoid heap allocation in loop
    char labelKey[20];
    char tzKey[20];
    snprintf(labelKey, sizeof(labelKey), "%s%dLabel", PREF_REMOTE_PREFIX, i);
    snprintf(tzKey, sizeof(tzKey), "%s%dTz", PREF_REMOTE_PREFIX, i);
    String label = prefs.getString(labelKey, DEFAULT_REMOTE_LABELS[i]);
    String tz = prefs.getString(tzKey, DEFAULT_REMOTE_TZS[i]);
    char cityBuf[32];
    extractCityName(label.c_str(), cityBuf, sizeof(cityBuf));
    strlcpy(config.remoteCities[i], cityBuf, sizeof(config.remoteCities[i]));
    strlcpy(config.remoteTzStrings[i], tz.c_str(), sizeof(config.remoteTzStrings[i]));
  }

  // Load display orientation
  config.landscapeMode = prefs.getBool(PREF_LANDSCAPE, false);  // Default: portrait

  prefs.end();
  DBG_INFO("Config loaded: Home=%s, Remote0=%s, Landscape=%d\n",
           config.homeCityLabel, config.remoteCities[0], config.landscapeMode);
}

// Save configuration to NVS
void saveConfig() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putString(PREF_HOME_LABEL, config.homeCityLabel);
  prefs.putString(PREF_HOME_TZ, config.homeCityTz);
  for (int i = 0; i < 5; i++) {
    prefs.putString((String(PREF_REMOTE_PREFIX) + i + "Label").c_str(), config.remoteCities[i]);
    prefs.putString((String(PREF_REMOTE_PREFIX) + i + "Tz").c_str(), config.remoteTzStrings[i]);
  }
  prefs.putBool(PREF_LANDSCAPE, config.landscapeMode);
  prefs.end();
  DBG_INFO("Config saved\n");
}

// Apply display rotation based on config
void applyRotation() {
  int rotation = config.landscapeMode ? 1 : 0;
  tft.setRotation(rotation);
  touchscreen.setRotation(rotation);
  DBG_INFO("Display rotation set to %d (%s)\n",
           rotation, config.landscapeMode ? "landscape" : "portrait");
}

// Color palette.
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_LABEL = TFT_WHITE;
static const uint16_t COLOR_TIME = TFT_GREEN;

// Layout + font settings (easy to tweak for readability).
// Portrait mode: 240x320
const int kTitleHeight = 22;   // "WORLD CLOCK" title row
const int kDateHeight = 18;    // Date display row
const int kHeaderHeight = kTitleHeight + kDateHeight;  // Total header = 40px
const int kPad = 8;
const int kBacklightPin = 21;
const bool kUseSmoothFonts = true;

// Landscape mode: 320x240
const int kLeftPanelWidth = 120;       // Title, date, home city (narrower)
const int kRightPanelWidth = 200;      // 5 remote cities (wider for times)
const int kLandscapeRemoteRowHeight = 48;  // 240 / 5 = 48px per remote city

// Analog clock settings (landscape mode left panel)
const int kClockCenterX = 60;          // Center of left panel (120/2)
const int kClockCenterY = 120;         // Centered between date (y~60) and digital time (y=185)
const int kClockRadius = 50;           // Clock face radius
const int kHourHandLen = 25;           // Hour hand length
const int kMinuteHandLen = 35;         // Minute hand length
const int kSecondHandLen = 40;         // Second hand length
const uint16_t kClockFaceColor = TFT_DARKGREY;
const uint16_t kHourMarkerColor = TFT_WHITE;
const uint16_t kHourHandColor = TFT_WHITE;
const uint16_t kMinuteHandColor = TFT_WHITE;
const uint16_t kSecondHandColor = TFT_RED;
const char *kFontHeader = "NotoSans-Bold9";
const char *kFontLabel = "NotoSans-Bold10";
const char *kFontTime = "NotoSans-Bold16";
const char *kFontNote = "NotoSans-Bold7";
const int kFallbackHeader = 2;
const int kFallbackLabel = 4;
const int kFallbackTime = 6;
const int kFallbackNote = 2;

// Forward declarations
void takeScreenshot();
void takeScreenshotRaw();

// Cached state to minimize redraws and flicker.
// Use fixed char arrays instead of String to avoid heap fragmentation
char lastDate[16];
char lastTimes[6][8];  // 6 cities: home + 5 remote, "HH:MM" format
bool lastPrevDay[6];
bool lastNextDay[6];
bool lastColonState[6];
int timePadWidth = 0;
unsigned long lastDebugPrint = 0;
bool smoothFontsReady = false;
const char *currentSmoothFont = nullptr;

// Analog clock state (for selective redraw)
int lastSecond = -1;
int lastMinute = -1;
int lastHour = -1;

// NOTE: Old getLocalTm() function removed - it used setenv() which leaks memory
// Now using getLocalTimeNoSetenv() with manual TZ calculation instead

// Switch between smooth fonts (loaded from SPIFFS) or fallback bitmap fonts.
// Optimized to avoid heap allocations - uses stack buffer for path construction.
void setFont(const char *smoothFontName, int fallbackFont) {
  if (kUseSmoothFonts && smoothFontsReady) {
    // Only reload if font actually changed (pointer comparison is intentional
    // since we use string literals for font names)
    if (currentSmoothFont != smoothFontName) {
      // Safely unload previous font if one was loaded
      if (currentSmoothFont != nullptr) {
        tft.unloadFont();
        currentSmoothFont = nullptr;
      }
      // Use stack buffer instead of String to avoid heap fragmentation
      char path[48];
      snprintf(path, sizeof(path), "/%s.vlw", smoothFontName);
      if (LittleFS.exists(path)) {
        tft.loadFont(smoothFontName, LittleFS);
        currentSmoothFont = smoothFontName;
        return;
      }
      // If smooth font failed to load, fall through to bitmap font
    } else if (currentSmoothFont != nullptr) {
      // Font already loaded and valid, nothing to do
      return;
    }
    // If we get here, either smooth font failed or currentSmoothFont is null
  }
  // Use bitmap font as fallback
  currentSmoothFont = nullptr;
  tft.setTextFont(fallbackFont);
}

// =========================
// Analog Clock Drawing (Landscape Mode)
// =========================

// Draw a clock hand from center to endpoint
// Uses line drawing - erases by drawing in background color
void drawClockHand(int cx, int cy, int length, float angleDeg, uint16_t color, int thickness = 1) {
  // Convert angle to radians (0 = 12 o'clock, clockwise)
  float angleRad = (angleDeg - 90.0f) * PI / 180.0f;
  int x2 = cx + (int)(length * cos(angleRad));
  int y2 = cy + (int)(length * sin(angleRad));

  if (thickness <= 1) {
    tft.drawLine(cx, cy, x2, y2, color);
  } else {
    // Draw thicker hand using multiple parallel lines
    for (int i = -thickness/2; i <= thickness/2; i++) {
      int offsetX = (int)(i * sin(angleRad));
      int offsetY = (int)(-i * cos(angleRad));
      tft.drawLine(cx + offsetX, cy + offsetY, x2 + offsetX, y2 + offsetY, color);
    }
  }
}

// Draw the static analog clock face (circle + hour markers)
void drawAnalogClockFace() {
  // Draw clock face circle
  tft.drawCircle(kClockCenterX, kClockCenterY, kClockRadius, kClockFaceColor);

  // Draw hour markers (12 positions)
  for (int i = 0; i < 12; i++) {
    float angleDeg = i * 30.0f;  // 360/12 = 30 degrees per hour
    float angleRad = (angleDeg - 90.0f) * PI / 180.0f;

    // Inner and outer positions for marker
    int outerR = kClockRadius - 3;
    int innerR = kClockRadius - 8;

    int x1 = kClockCenterX + (int)(innerR * cos(angleRad));
    int y1 = kClockCenterY + (int)(innerR * sin(angleRad));
    int x2 = kClockCenterX + (int)(outerR * cos(angleRad));
    int y2 = kClockCenterY + (int)(outerR * sin(angleRad));

    // Draw marker (thicker at 12, 3, 6, 9)
    if (i % 3 == 0) {
      tft.drawLine(x1, y1, x2, y2, kHourMarkerColor);
      tft.drawLine(x1+1, y1, x2+1, y2, kHourMarkerColor);
    } else {
      tft.drawLine(x1, y1, x2, y2, kClockFaceColor);
    }
  }

  // Draw center dot
  tft.fillCircle(kClockCenterX, kClockCenterY, 3, kHourMarkerColor);
}

// Update analog clock hands (selective redraw for flicker-free animation)
void updateAnalogClockHands(int hour, int minute, int second) {
  // Calculate angles
  // Hour: 30 degrees per hour + 0.5 degrees per minute
  float hourAngle = (hour % 12) * 30.0f + minute * 0.5f;
  // Minute: 6 degrees per minute
  float minuteAngle = minute * 6.0f;
  // Second: 6 degrees per second
  float secondAngle = second * 6.0f;

  // Erase old hands if they've changed (draw in background color)
  // Order matters: erase in reverse order of drawing (second, minute, hour)
  if (lastSecond >= 0 && lastSecond != second) {
    float oldSecondAngle = lastSecond * 6.0f;
    drawClockHand(kClockCenterX, kClockCenterY, kSecondHandLen, oldSecondAngle, COLOR_BG, 1);
  }
  if (lastMinute >= 0 && lastMinute != minute) {
    float oldMinuteAngle = lastMinute * 6.0f;
    drawClockHand(kClockCenterX, kClockCenterY, kMinuteHandLen, oldMinuteAngle, COLOR_BG, 2);
    // Also need to erase hour hand since it moves with minutes
    float oldHourAngle = (lastHour % 12) * 30.0f + lastMinute * 0.5f;
    drawClockHand(kClockCenterX, kClockCenterY, kHourHandLen, oldHourAngle, COLOR_BG, 3);
  }

  // Draw new hands (order: hour, minute, second - so second is on top)
  if (lastMinute != minute || lastHour != hour) {
    drawClockHand(kClockCenterX, kClockCenterY, kHourHandLen, hourAngle, kHourHandColor, 3);
    drawClockHand(kClockCenterX, kClockCenterY, kMinuteHandLen, minuteAngle, kMinuteHandColor, 2);
  }
  drawClockHand(kClockCenterX, kClockCenterY, kSecondHandLen, secondAngle, kSecondHandColor, 1);

  // Redraw center dot (may have been partially erased)
  tft.fillCircle(kClockCenterX, kClockCenterY, 3, kHourMarkerColor);

  // Update state
  lastSecond = second;
  lastMinute = minute;
  lastHour = hour;
}

// Block until NTP has set a valid time (returns false on timeout).
bool syncTime() {
  configTzTime(config.homeCityTz, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  for (int i = 0; i < 20; ++i) {
    if (getLocalTime(&timeinfo)) {
      DBG_INFO("NTP synced to %s\n", config.homeCityTz);
      return true;
    }
    delay(10);
    yield();
    delay(500);
  }
  return false;
}

// Read LDR (Light Dependent Resistor) value
// Returns averaged ADC value 0-4095 (12-bit on ESP32)
// Takes 10 samples and averages to reduce noise
int readLDR() {
  uint32_t sum = 0;
  const int samples = 10;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(LDR_PIN);
    delay(1);  // Small delay between samples
  }

  return sum / samples;
}

// Optional: list files on LittleFS to confirm fonts are present.
void logLittleFSContents() {
  if (!smoothFontsReady) {
    return;
  }
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    DBG_WARN("LittleFS: open root failed\n");
    return;
  }
  DBG_INFO("LittleFS files:\n");
  File file = root.openNextFile();
  while (file) {
    DBG_INFO(" - %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

// Format a date string for home city (e.g., "THU 24 MAR").
// Writes directly to provided buffer to avoid heap allocation.
// Uses manual TZ calculation - NO setenv() calls, NO memory leak!
void formatDate(char *outBuf, size_t bufSize) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  getLocalTimeNoSetenv(now, &parsedTz[0], &timeinfo);  // Always use home city
  strftime(outBuf, bufSize, "%a %d %b", &timeinfo);
  // Convert to uppercase in place
  for (char *p = outBuf; *p; ++p) {
    *p = toupper((unsigned char)*p);
  }
}

struct TimeInfo {
  char timeStr[8];  // "HH:MM" + null terminator - fixed size to avoid heap allocation
  bool prevDay;
  bool nextDay;
  bool showColon;
};

// CRITICAL FIX v2: Batch all timezone updates to minimize setenv() calls
// setenv("TZ") leaks ~30-40 bytes per call on ESP32
// By batching updates, we reduce from 6 calls/minute to 1 call/minute
struct CachedTimeInfo {
  struct tm tm;
  char timeStr[8];
  bool prevDay;
  bool nextDay;
};

static CachedTimeInfo timeCache[6];  // Cache for all 6 cities (home + 5 remote)
static time_t lastBatchUpdate = 0;   // Last time we updated ALL cities
static bool timeCacheInitialized = false;

// Update all city times in a single batch - called once per minute
// Uses manual TZ calculation - NO setenv() calls, NO memory leak!
void updateAllCityTimes() {
  time_t now = time(nullptr);

  // Get home city time first (needed for prevDay/nextDay comparison)
  getLocalTimeNoSetenv(now, &parsedTz[0], &timeCache[0].tm);
  snprintf(timeCache[0].timeStr, sizeof(timeCache[0].timeStr),
           "%02d:%02d", timeCache[0].tm.tm_hour, timeCache[0].tm.tm_min);
  timeCache[0].prevDay = false;  // Home is never "previous day" relative to itself
  timeCache[0].nextDay = false;  // Home is never "next day" relative to itself

  // Update remote cities
  for (int i = 0; i < 5; i++) {
    getLocalTimeNoSetenv(now, &parsedTz[i + 1], &timeCache[i + 1].tm);
    snprintf(timeCache[i + 1].timeStr, sizeof(timeCache[i + 1].timeStr),
             "%02d:%02d", timeCache[i + 1].tm.tm_hour, timeCache[i + 1].tm.tm_min);

    // Check if remote city is in previous day relative to home
    timeCache[i + 1].prevDay = false;
    timeCache[i + 1].nextDay = false;
    if (timeCache[i + 1].tm.tm_year < timeCache[0].tm.tm_year) {
      timeCache[i + 1].prevDay = true;
    } else if (timeCache[i + 1].tm.tm_year == timeCache[0].tm.tm_year &&
               timeCache[i + 1].tm.tm_yday < timeCache[0].tm.tm_yday) {
      timeCache[i + 1].prevDay = true;
    }
    // Check if remote city is in next day relative to home
    if (timeCache[i + 1].tm.tm_year > timeCache[0].tm.tm_year) {
      timeCache[i + 1].nextDay = true;
    } else if (timeCache[i + 1].tm.tm_year == timeCache[0].tm.tm_year &&
               timeCache[i + 1].tm.tm_yday > timeCache[0].tm.tm_yday) {
      timeCache[i + 1].nextDay = true;
    }
  }

  // No setenv() restore needed - we don't use setenv() anymore!

  lastBatchUpdate = now;
  timeCacheInitialized = true;
}

// Build time string plus flags used by the renderer.
// Uses cached values - only updates when minute changes via batch update
// NO setenv() calls - uses manual TZ calculation
TimeInfo formatTime(int cityIndex) {
  time_t now = time(nullptr);
  TimeInfo info;

  // Check if we need to do a batch update (minute changed or first call)
  if (lastBatchUpdate == 0 || (now / 60) != (lastBatchUpdate / 60)) {
    updateAllCityTimes();
  }

  // Use cached values
  strcpy(info.timeStr, timeCache[cityIndex].timeStr);
  info.prevDay = timeCache[cityIndex].prevDay;
  info.nextDay = timeCache[cityIndex].nextDay;
  info.showColon = (now % 2 == 0);  // Blink every second

  return info;
}

// Draw static layout for portrait mode (240x320)
void drawStaticLayoutPortrait() {
  // Draw centered "WORLD CLOCK" title on first row
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  setFont(kFontHeader, kFallbackHeader);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WORLD CLOCK", tft.width() / 2, kTitleHeight / 2 + 4);

  int rows = 6;  // Home + 5 remote cities
  int rowHeight = (tft.height() - kHeaderHeight) / rows;

  // Draw city labels starting below header
  setFont(kFontLabel, kFallbackLabel);
  tft.setTextDatum(TL_DATUM);

  // Home city (row 0) with "Home" label
  int rowTop = kHeaderHeight;
  tft.drawString(config.homeCityLabel, kPad, rowTop + 2);

  // Add small "Home" label beneath city name
  setFont(kFontNote, kFallbackNote);
  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.drawString("HOME", kPad, rowTop + 2 + tft.fontHeight() + 4);

  // Restore label font and color for remote cities
  setFont(kFontLabel, kFallbackLabel);
  tft.setTextColor(COLOR_LABEL, COLOR_BG);

  // Remote cities (rows 1-5)
  for (int i = 0; i < 5; ++i) {
    rowTop = kHeaderHeight + (i + 1) * rowHeight;
    int labelY = rowTop + 2;
    tft.drawString(config.remoteCities[i], kPad, labelY);
  }
}

// Draw static layout for landscape mode (320x240)
// Left panel (120px): City name, HOME, date, analog clock, digital time
// Right panel (200px): 5 remote cities stacked vertically with times
void drawStaticLayoutLandscape() {
  // LEFT PANEL layout: City (y=6) → HOME (y=30) → Date (y=48) → Clock (y=120) → Time (y=185)

  // Home city label (at top) - use smaller font for long names (>9 chars)
  int cityLen = strlen(config.homeCityLabel);
  if (cityLen > 9) {
    setFont(kFontNote, kFallbackNote);  // Smaller font for long names
  } else {
    setFont(kFontLabel, kFallbackLabel);
  }
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  tft.drawString(config.homeCityLabel, kLeftPanelWidth / 2, 6);

  // "HOME" indicator below city name
  setFont(kFontNote, kFallbackNote);
  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("HOME", kLeftPanelWidth / 2, 30);

  // Date will be drawn by drawHeaderDate() at y=48

  // Draw analog clock face (centered at y=120)
  drawAnalogClockFace();

  // Digital time will be drawn by drawTimesLandscape() at Y=185

  // Divider line between left and right panels
  tft.drawFastVLine(kLeftPanelWidth - 1, 0, tft.height(), TFT_DARKGREY);

  // RIGHT PANEL: 5 Remote Cities (no horizontal lines)
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  tft.setTextDatum(TL_DATUM);

  for (int i = 0; i < 5; i++) {
    int rowY = i * kLandscapeRemoteRowHeight + 2;  // City at top of row
    // Use smaller font for long city names (>9 chars)
    int remoteCityLen = strlen(config.remoteCities[i]);
    if (remoteCityLen > 9) {
      setFont(kFontNote, kFallbackNote);
    } else {
      setFont(kFontLabel, kFallbackLabel);
    }
    tft.drawString(config.remoteCities[i], kLeftPanelWidth + kPad, rowY);
  }
}

// Draw the static header and location labels once.
void drawStaticLayout() {
  tft.fillScreen(COLOR_BG);

  if (config.landscapeMode) {
    drawStaticLayoutLandscape();
  } else {
    drawStaticLayoutPortrait();
  }
}

// Draw or update the header date string.
void drawHeaderDate(const char *dateStr) {
  setFont(kFontHeader, kFallbackHeader);
  tft.setTextColor(COLOR_TIME, COLOR_BG);

  if (config.landscapeMode) {
    // Landscape: date in left panel, below HOME indicator (y=48)
    tft.setTextDatum(TC_DATUM);
    tft.fillRect(0, 46, kLeftPanelWidth - 2, 18, COLOR_BG);  // Clear date area
    tft.drawString(dateStr, kLeftPanelWidth / 2, 48);
  } else {
    // Portrait: centered date below title (full width)
    tft.setTextDatum(MC_DATUM);
    tft.fillRect(0, kTitleHeight, tft.width(), kDateHeight, COLOR_BG);
    tft.drawString(dateStr, tft.width() / 2, kTitleHeight + kDateHeight / 2 + 2);
  }
}

// Draw times for portrait mode (240x320)
void drawTimesPortrait() {
  int rows = 6;  // Home + 5 remote cities
  int rowHeight = (tft.height() - kHeaderHeight) / rows;

  setFont(kFontTime, kFallbackTime);
  if (timePadWidth == 0) {
    timePadWidth = tft.textWidth("88:88");
  }
  tft.setTextPadding(timePadWidth);
  tft.setTextColor(COLOR_TIME, COLOR_BG);
  tft.setTextDatum(TR_DATUM);

  // Helper to get city label by index
  auto getLabelByIndex = [](int i) -> const char* {
    if (i == 0) return config.homeCityLabel;
    return config.remoteCities[i - 1];
  };

  for (int i = 0; i < rows; ++i) {
    tft.setTextPadding(timePadWidth);
    TimeInfo info = formatTime(i);
    bool timeChanged = (strcmp(info.timeStr, lastTimes[i]) != 0);
    bool prevDayChanged = (info.prevDay != lastPrevDay[i]);
    bool nextDayChanged = (info.nextDay != lastNextDay[i]);
    bool colonChanged = (info.showColon != lastColonState[i]);
    if (!timeChanged && !prevDayChanged && !nextDayChanged && !colonChanged) {
      continue;
    }

    int rowTop = kHeaderHeight + i * rowHeight;
    int timeY = rowTop + 2;

    if (timeChanged || prevDayChanged || nextDayChanged || colonChanged) {
      setFont(kFontTime, kFallbackTime);
      int timeWidth = tft.textWidth("88:88");
      int clearX = (tft.width() - kPad) - timeWidth - 2;
      tft.fillRect(clearX, timeY, timeWidth + 4, tft.fontHeight(), COLOR_BG);
      tft.setTextDatum(TR_DATUM);

      // Build time string with colon or space for blinking
      char displayTime[8];
      strcpy(displayTime, info.timeStr);
      if (!info.showColon) {
        displayTime[2] = ' ';  // Replace colon with space
      }
      tft.drawString(displayTime, tft.width() - kPad, timeY);
    }

    if (prevDayChanged || nextDayChanged) {
      int labelClearWidth = tft.width() / 2 - kPad - 4;
      tft.fillRect(kPad, rowTop, labelClearWidth, rowHeight, COLOR_BG);
      setFont(kFontLabel, kFallbackLabel);
      tft.setTextColor(COLOR_LABEL, COLOR_BG);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(getLabelByIndex(i), kPad, rowTop + 2);
      if (info.prevDay) {
        setFont(kFontNote, kFallbackNote);
        tft.setTextColor(TFT_YELLOW, COLOR_BG);
        tft.drawString("Prev Day", kPad, rowTop + 2 + tft.fontHeight() + 2);
      } else if (info.nextDay) {
        setFont(kFontNote, kFallbackNote);
        tft.setTextColor(TFT_CYAN, COLOR_BG);
        tft.drawString("Next Day", kPad, rowTop + 2 + tft.fontHeight() + 2);
      }
    }

    setFont(kFontTime, kFallbackTime);
    tft.setTextColor(COLOR_TIME, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    strlcpy(lastTimes[i], info.timeStr, sizeof(lastTimes[i]));
    lastPrevDay[i] = info.prevDay;
    lastNextDay[i] = info.nextDay;
    lastColonState[i] = info.showColon;
  }
}

// Draw times for landscape mode (320x240)
// Left panel: Analog clock + digital time below
// Right panel: Times right-aligned, PREV DAY tiny below city label
void drawTimesLandscape() {
  setFont(kFontTime, kFallbackTime);
  if (timePadWidth == 0) {
    timePadWidth = tft.textWidth("88:88");
  }

  // Get home city time for analog clock
  time_t now = time(nullptr);
  struct tm homeTm;
  getLocalTimeNoSetenv(now, &parsedTz[0], &homeTm);

  // Update analog clock hands (every second)
  updateAnalogClockHands(homeTm.tm_hour, homeTm.tm_min, homeTm.tm_sec);

  // HOME CITY DIGITAL TIME (left panel, below analog clock)
  {
    TimeInfo info = formatTime(0);
    bool timeChanged = (strcmp(info.timeStr, lastTimes[0]) != 0);
    bool colonChanged = (info.showColon != lastColonState[0]);

    if (timeChanged || colonChanged) {
      int homeTimeY = 185;  // Below analog clock (center Y=120, radius=50)

      setFont(kFontTime, kFallbackTime);
      tft.setTextColor(COLOR_TIME, COLOR_BG);
      tft.setTextDatum(TC_DATUM);
      // Clear home time area
      tft.fillRect(kPad, homeTimeY, kLeftPanelWidth - kPad * 2 - 2, 30, COLOR_BG);

      // Build time string with colon or space for blinking
      char displayTime[8];
      strcpy(displayTime, info.timeStr);
      if (!info.showColon) {
        displayTime[2] = ' ';  // Replace colon with space
      }
      tft.drawString(displayTime, kLeftPanelWidth / 2, homeTimeY);

      strlcpy(lastTimes[0], info.timeStr, sizeof(lastTimes[0]));
      lastPrevDay[0] = info.prevDay;
      lastNextDay[0] = info.nextDay;
      lastColonState[0] = info.showColon;
    }
  }

  // REMOTE CITIES TIMES (right panel - 200px wide)
  // Layout: City label at top, Time below (right-aligned), PREV/NEXT DAY at bottom
  for (int i = 0; i < 5; i++) {
    int cityIndex = i + 1;
    TimeInfo info = formatTime(cityIndex);
    bool timeChanged = (strcmp(info.timeStr, lastTimes[cityIndex]) != 0);
    bool prevDayChanged = (info.prevDay != lastPrevDay[cityIndex]);
    bool nextDayChanged = (info.nextDay != lastNextDay[cityIndex]);
    bool colonChanged = (info.showColon != lastColonState[cityIndex]);

    if (!timeChanged && !prevDayChanged && !nextDayChanged && !colonChanged) {
      continue;
    }

    int rowY = i * kLandscapeRemoteRowHeight;
    int cityLabelY = rowY + 2;     // City label at top
    int timeY = rowY + 20;         // Time right-aligned

    if (timeChanged || prevDayChanged || nextDayChanged) {
      // Clear the entire row area for this city (except divider line)
      tft.fillRect(kLeftPanelWidth + 1, rowY, kRightPanelWidth - 1, kLandscapeRemoteRowHeight, COLOR_BG);

      // Redraw city label
      setFont(kFontLabel, kFallbackLabel);
      tft.setTextColor(COLOR_LABEL, COLOR_BG);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(config.remoteCities[i], kLeftPanelWidth + kPad, cityLabelY);

      // Draw PREV DAY or NEXT DAY indicator
      if (info.prevDay) {
        setFont(kFontNote, kFallbackNote);
        tft.setTextColor(TFT_YELLOW, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("PREV DAY", kLeftPanelWidth + kPad, cityLabelY + tft.fontHeight() + 2);
      } else if (info.nextDay) {
        setFont(kFontNote, kFallbackNote);
        tft.setTextColor(TFT_CYAN, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("NEXT DAY", kLeftPanelWidth + kPad, cityLabelY + tft.fontHeight() + 2);
      }
    }

    if (timeChanged || colonChanged) {
      // Draw time right-aligned (with colon or space for blinking)
      setFont(kFontTime, kFallbackTime);
      int timeWidth = tft.textWidth("88:88");
      tft.fillRect(tft.width() - 6 - timeWidth - 2, timeY, timeWidth + 4, tft.fontHeight(), COLOR_BG);
      tft.setTextColor(COLOR_TIME, COLOR_BG);
      tft.setTextDatum(TR_DATUM);

      // Build time string with colon or space for blinking
      char displayTime[8];
      strcpy(displayTime, info.timeStr);
      if (!info.showColon) {
        displayTime[2] = ' ';  // Replace colon with space
      }
      tft.drawString(displayTime, tft.width() - 6, timeY);
    }

    strlcpy(lastTimes[cityIndex], info.timeStr, sizeof(lastTimes[cityIndex]));
    lastPrevDay[cityIndex] = info.prevDay;
    lastNextDay[cityIndex] = info.nextDay;
    lastColonState[cityIndex] = info.showColon;
  }
}

// Draw times for each location and update only when needed.
// Uses cached time values - NO setenv() calls
void drawTimes() {
  if (config.landscapeMode) {
    drawTimesLandscape();
  } else {
    drawTimesPortrait();
  }
}

// =========================
// WiFi & OTA Setup
// =========================

static void displayWiFiSetupInstructions(const char* apName, const char* ipAddress) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);

  // Title
  setFont("NotoSans-Bold16", 4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("WiFi SETUP", tft.width() / 2, 20);

  // Instructions
  setFont("NotoSans-Bold10", 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  int y = 60;
  int x = 10;
  int lineHeight = 24;

  tft.drawString("1. Connect to WiFi:", x, y);
  y += lineHeight;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(apName, x + 20, y);
  y += lineHeight + 10;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("2. Browser opens auto", x, y);
  y += lineHeight;
  tft.drawString("   or go to:", x, y);
  y += lineHeight;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(ipAddress, x + 20, y);
  y += lineHeight + 10;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("3. Select your WiFi", x, y);
  y += lineHeight;
  tft.drawString("   and enter password", x, y);
  y += lineHeight + 10;

  // Footer note
  setFont("NotoSans-Bold7", 1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("Portal stays open until configured", tft.width() / 2, tft.height() - 10);
}

static void configModeCallback(WiFiManager* myWiFiManager) {
  DBG_INFO("Entered WiFi config mode\n");
  DBG_INFO("Connect to AP: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  DBG_INFO("Config portal IP: %s\n", WiFi.softAPIP().toString().c_str());

  // Display setup instructions on TFT
  displayWiFiSetupInstructions(
    myWiFiManager->getConfigPortalSSID().c_str(),
    WiFi.softAPIP().toString().c_str()
  );
}

// Forward declaration
void updateWiFiCache();

static void startWifi() {
  DBG_STEP("Starting WiFi (STA) + WiFiManager...");
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  // No timeout for config portal - stay open until WiFi is configured
  // This prevents the portal from closing if user doesn't respond quickly
  wm.setConfigPortalTimeout(0);
  wm.setConnectTimeout(20);
  wm.setAPCallback(configModeCallback);

  // Add handlers to WiFiManager's web server to suppress browser auto-request errors
  wm.setWebServerCallback([&wm]() {
    // WiFiManager's internal web server
    wm.server->on("/favicon.ico", [&wm]() {
      wm.server->send(204);  // No content
    });
    wm.server->on("/generate_204", [&wm]() {  // Android captive portal check
      wm.server->send(204);
    });
    wm.server->on("/hotspot-detect.html", [&wm]() {  // iOS captive portal check
      wm.server->send(204);
    });
    wm.server->on("/library/test/success.html", [&wm]() {  // iOS captive portal check
      wm.server->send(204);
    });
    wm.server->on("/connecttest.txt", [&wm]() {  // Windows captive portal check
      wm.server->send(204);
    });
  });

  bool ok = wm.autoConnect("WorldClock-Setup");
  if (!ok) {
    DBG_WARN("WiFiManager autoConnect failed/timeout. Starting fallback AP...\n");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WorldClock-AP");
    // Display fallback AP instructions
    displayWiFiSetupInstructions("WorldClock-AP", WiFi.softAPIP().toString().c_str());
  }

  if (WiFi.isConnected()) {
    updateWiFiCache();  // Cache WiFi info to prevent String allocations in API handlers
    DBG_INFO("WiFi connected: SSID=%s IP=%s\n", cachedSSID, cachedIP);
    DBG_OK("WiFi ready.");
  } else {
    DBG_WARN("WiFi not connected (AP mode).\n");
  }
}

static void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    DBG_INFO("OTA: Update starting...\n");
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("OTA UPDATE", tft.width() / 2, tft.height() / 2 - 30);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = (progress * 100) / total;
    static int lastPct = -1;
    if (pct != lastPct) {
      DBG_INFO("OTA Progress: %d%%\n", pct);
      // Draw progress bar
      int barWidth = 200;
      int barHeight = 20;
      int barX = (tft.width() - barWidth) / 2;
      int barY = tft.height() / 2;
      tft.drawRect(barX - 2, barY - 2, barWidth + 4, barHeight + 4, TFT_WHITE);
      int fillWidth = (barWidth * pct) / 100;
      tft.fillRect(barX, barY, fillWidth, barHeight, TFT_GREEN);
      lastPct = pct;
    }
  });

  ArduinoOTA.onEnd([]() {
    DBG_INFO("OTA: Update complete!\n");
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("UPDATE", tft.width() / 2, tft.height() / 2 - 15);
    tft.drawString("COMPLETE", tft.width() / 2, tft.height() / 2 + 15);
    delay(1000);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DBG_ERROR("OTA Error[%u]: ", error);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("OTA FAILED", tft.width() / 2, tft.height() / 2);
    delay(3000);
  });

  ArduinoOTA.begin();
  DBG_OK("OTA ready");
}

// =========================
// WebUI API Endpoints
// =========================

// Update cached WiFi info (call after WiFi connect)
void updateWiFiCache() {
  strlcpy(cachedSSID, WiFi.SSID().c_str(), sizeof(cachedSSID));
  strlcpy(cachedIP, WiFi.localIP().toString().c_str(), sizeof(cachedIP));
  cachedRSSI = WiFi.RSSI();
}

// GET /api/state - Return current configuration as JSON
// OPTIMIZED: Uses JsonDocument and cached WiFi info to prevent memory leaks
void handleGetState() {
  DBG_VERBOSE("GET /api/state\n");

  JsonDocument doc;

  // System info
  doc["firmware"] = FIRMWARE_VERSION;
  doc["hostname"] = OTA_HOSTNAME;
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["debugLevel"] = debugLevel;
  doc["ldrValue"] = readLDR();
  doc["landscapeMode"] = config.landscapeMode;

  // WiFi info - use cached values to avoid String allocations
  doc["wifi_ssid"] = cachedSSID;
  doc["wifi_ip"] = cachedIP;
  doc["wifi_rssi"] = cachedRSSI;

  // Home city config
  JsonObject homeCity = doc["homeCity"].to<JsonObject>();
  homeCity["label"] = config.homeCityLabel;
  homeCity["tz"] = config.homeCityTz;

  // Remote cities config
  JsonArray remoteCities = doc["remoteCities"].to<JsonArray>();
  for (int i = 0; i < 5; i++) {
    JsonObject city = remoteCities.add<JsonObject>();
    city["label"] = config.remoteCities[i];
    city["tz"] = config.remoteTzStrings[i];
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// POST /api/debug-level - Set debug level (0-4)
void handleSetDebugLevel() {
  DBG_VERBOSE("POST /api/debug-level\n");

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing request body");
    return;
  }

  JsonDocument doc;
  auto err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  if (!doc["level"].is<int>()) {
    server.send(400, "text/plain", "Missing level field");
    return;
  }

  int level = doc["level"];
  DBG_INFO("POST /api/debug-level: %d\n", level);

  if (level >= 0 && level <= 4) {
    debugLevel = level;
    DBG_INFO("Debug level set to %d\n", debugLevel);

    JsonDocument response;
    response["success"] = true;
    response["debugLevel"] = debugLevel;

    String output;
    serializeJson(response, output);
    server.send(200, "application/json", output);
  } else {
    server.send(400, "text/plain", "Invalid level (0-4)");
  }
}

// POST /api/config - Update configuration
void handlePostConfig() {
  DBG_INFO("POST /api/config\n");

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing request body");
    return;
  }

  JsonDocument doc;
  auto err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  // Parse home city
  if (!doc["homeCity"].isNull()) {
    if (!doc["homeCity"]["label"].isNull()) {
      const char* homeLabel = doc["homeCity"]["label"];
      // FIXED: Use char buffer instead of String to avoid allocation
      char cityBuf[32];
      extractCityName(homeLabel, cityBuf, sizeof(cityBuf));
      strlcpy(config.homeCityLabel, cityBuf, sizeof(config.homeCityLabel));
      DBG_INFO("  Home city: %s\n", config.homeCityLabel);
    }
    if (!doc["homeCity"]["tz"].isNull()) {
      const char* homeTz = doc["homeCity"]["tz"];
      strlcpy(config.homeCityTz, homeTz, sizeof(config.homeCityTz));
    }
  }

  // Parse remote cities
  if (!doc["remoteCities"].isNull()) {
    JsonArray cities = doc["remoteCities"].as<JsonArray>();
    int i = 0;
    for (JsonVariant city : cities) {
      if (i >= 5) break;

      if (!city["label"].isNull()) {
        const char* label = city["label"];
        // FIXED: Use char buffer instead of String to avoid allocation
        char cityBuf[32];
        extractCityName(label, cityBuf, sizeof(cityBuf));
        strlcpy(config.remoteCities[i], cityBuf, sizeof(config.remoteCities[i]));
      }
      if (!city["tz"].isNull()) {
        const char* tz = city["tz"];
        strlcpy(config.remoteTzStrings[i], tz, sizeof(config.remoteTzStrings[i]));
      }
      i++;
    }
  }

  // Parse display orientation
  bool rotationChanged = false;
  if (!doc["landscapeMode"].isNull()) {
    bool newLandscape = doc["landscapeMode"].as<bool>();
    if (config.landscapeMode != newLandscape) {
      config.landscapeMode = newLandscape;
      rotationChanged = true;
      DBG_INFO("  Display mode: %s\n", newLandscape ? "landscape" : "portrait");
    }
  }

  saveConfig();
  loadConfig();  // Reload config immediately - no reboot needed
  parseAllTimezones();  // Re-parse TZ strings for manual calculation

  // Apply rotation if changed
  if (rotationChanged) {
    applyRotation();
  }

  // Redraw static layout with new city labels
  drawStaticLayout();

  // Reset cached state to force full redraw of times on next loop
  lastDate[0] = '\0';
  for (int i = 0; i < 6; i++) {
    lastTimes[i][0] = '\0';
    lastPrevDay[i] = false;
    lastNextDay[i] = false;
    lastColonState[i] = false;
  }
  // Reset analog clock state
  lastSecond = -1;
  lastMinute = -1;
  lastHour = -1;
  // Force immediate recalculation of time cache (including prevDay/nextDay)
  lastBatchUpdate = 0;

  server.send(200, "application/json", "{\"ok\":true}");
  DBG_INFO("Config updated and reloaded\n");
}

// POST /api/reset-wifi - Clear WiFi credentials
void handleResetWiFi() {
  DBG_INFO("POST /api/reset-wifi\n");
  server.send(200, "text/plain", "WiFi reset. Rebooting...");
  delay(1000);
  WiFiManager wm;
  wm.resetSettings();
  delay(1000);
  ESP.restart();
}

void handleReboot() {
  DBG_INFO("POST /api/reboot\n");
  server.send(200, "text/plain", "Rebooting device...");
  delay(1000);
  ESP.restart();
}

// GET /api/screenshot - Trigger screenshot capture (serial output)
void handleScreenshot() {
  DBG_INFO("GET /api/screenshot\n");
  server.send(200, "text/plain", "Screenshot will be sent via serial. Monitor serial output.");
  delay(500);  // Give web response time to send
  takeScreenshot();
}

// GET /api/snapshot - Stream BMP image of current display
// Streams directly to client to avoid memory issues
void handleSnapshot() {
  DBG_INFO("GET /api/snapshot - Capturing display as BMP\n");

  // Wait for colons to be visible (even second = colon shown)
  // Maximum wait of 1.1 seconds to ensure we catch the next even second
  time_t startWait = time(nullptr);
  while ((time(nullptr) % 2) != 0) {
    delay(100);
    if (time(nullptr) - startWait > 2) break;  // Safety timeout
  }
  // Small additional delay to ensure display has updated
  delay(100);

  int width = tft.width();
  int height = tft.height();

  // BMP row padding (rows must be multiple of 4 bytes)
  int rowSize = ((width * 3 + 3) / 4) * 4;
  int imageSize = rowSize * height;
  int fileSize = 54 + imageSize;  // 54-byte header + pixel data

  // BMP Header (14 bytes) + DIB Header (40 bytes) = 54 bytes
  uint8_t header[54] = {0};

  // BMP File Header (14 bytes)
  header[0] = 'B';
  header[1] = 'M';
  header[2] = fileSize & 0xFF;
  header[3] = (fileSize >> 8) & 0xFF;
  header[4] = (fileSize >> 16) & 0xFF;
  header[5] = (fileSize >> 24) & 0xFF;
  header[10] = 54;  // Pixel data offset

  // DIB Header (BITMAPINFOHEADER - 40 bytes)
  header[14] = 40;  // DIB header size
  header[18] = width & 0xFF;
  header[19] = (width >> 8) & 0xFF;
  header[22] = height & 0xFF;
  header[23] = (height >> 8) & 0xFF;
  header[26] = 1;   // Color planes
  header[28] = 24;  // Bits per pixel (RGB888)
  header[34] = imageSize & 0xFF;
  header[35] = (imageSize >> 8) & 0xFF;
  header[36] = (imageSize >> 16) & 0xFF;
  header[37] = (imageSize >> 24) & 0xFF;

  // Send HTTP headers
  server.sendHeader("Content-Type", "image/bmp");
  server.sendHeader("Content-Disposition", "attachment; filename=\"clock_snapshot.bmp\"");
  server.sendHeader("Content-Length", String(fileSize));
  server.sendHeader("Cache-Control", "no-cache");

  // Use chunked transfer
  WiFiClient client = server.client();
  client.write("HTTP/1.1 200 OK\r\n");
  client.write("Content-Type: image/bmp\r\n");
  client.write("Content-Disposition: attachment; filename=\"clock_snapshot.bmp\"\r\n");
  client.printf("Content-Length: %d\r\n", fileSize);
  client.write("Connection: close\r\n\r\n");

  // Send BMP header
  client.write(header, 54);

  // Allocate row buffer (width * 3 bytes for RGB + padding)
  uint8_t* rowBuf = (uint8_t*)malloc(rowSize);
  if (!rowBuf) {
    DBG_ERROR("Failed to allocate row buffer for snapshot\n");
    return;
  }

  // BMP is bottom-up, so read from bottom row to top
  for (int y = height - 1; y >= 0; y--) {
    int bufIdx = 0;
    for (int x = 0; x < width; x++) {
      uint16_t color565 = tft.readPixel(x, y);

      // Convert RGB565 to BGR888 (BMP uses BGR order)
      uint8_t r = ((color565 >> 11) & 0x1F) << 3;
      uint8_t g = ((color565 >> 5) & 0x3F) << 2;
      uint8_t b = (color565 & 0x1F) << 3;

      rowBuf[bufIdx++] = b;  // Blue
      rowBuf[bufIdx++] = g;  // Green
      rowBuf[bufIdx++] = r;  // Red
    }

    // Pad row to multiple of 4 bytes
    while (bufIdx < rowSize) {
      rowBuf[bufIdx++] = 0;
    }

    client.write(rowBuf, rowSize);
    yield();  // Allow WiFi stack to process
  }

  free(rowBuf);
  DBG_INFO("Snapshot complete: %dx%d, %d bytes\n", width, height, fileSize);
}

// GET /api/mirror - Return current clock state as JSON
// Text-only display mirror - FIXED to use JsonDocument instead of String concatenation
void handleMirror() {
  DBG_VERBOSE("GET /api/mirror\n");

  JsonDocument doc;

  // Get current time for home city using manual TZ calculation (no setenv leak)
  time_t now = time(nullptr);
  struct tm homeTm;
  getLocalTimeNoSetenv(now, &parsedTz[0], &homeTm);

  // Display mode
  doc["landscapeMode"] = config.landscapeMode;

  // Date from home city (uppercase for display)
  char dateStr[32];
  strftime(dateStr, sizeof(dateStr), "%a %d %b", &homeTm);
  // Convert to uppercase to match TFT display
  for (int i = 0; dateStr[i]; i++) {
    dateStr[i] = toupper(dateStr[i]);
  }
  doc["date"] = dateStr;

  // Clock angles for analog clock (landscape mode)
  // Hour: 30° per hour + 0.5° per minute (smooth movement)
  // Minute: 6° per minute
  // Second: 6° per second
  JsonObject clock = doc["clock"].to<JsonObject>();
  clock["hour"] = homeTm.tm_hour;
  clock["minute"] = homeTm.tm_min;
  clock["second"] = homeTm.tm_sec;

  // Home city
  JsonObject homeCity = doc["home"].to<JsonObject>();
  homeCity["label"] = config.homeCityLabel;
  homeCity["time"] = lastTimes[0];
  homeCity["prevDay"] = lastPrevDay[0];
  homeCity["nextDay"] = lastNextDay[0];

  // Remote cities
  JsonArray remoteCities = doc["remote"].to<JsonArray>();
  for (int i = 0; i < 5; i++) {
    JsonObject city = remoteCities.add<JsonObject>();
    city["label"] = config.remoteCities[i];
    city["time"] = lastTimes[i + 1];
    city["prevDay"] = lastPrevDay[i + 1];
    city["nextDay"] = lastNextDay[i + 1];
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
  DBG_VERBOSE("Mirror sent: %u bytes\n", output.length());
}

// GET /api/debug - Return recent logs
void handleDebug() {
  DBG_VERBOSE("GET /api/debug\n");

  // Size: logCount(4) + 20 logs × (timestamp(4) + level(5) + message(80)) = ~1800 bytes
  JsonDocument doc;
  doc["logCount"] = logCount;

  JsonArray logs = doc["logs"].to<JsonArray>();

  // Output logs in chronological order (oldest to newest)
  int startIdx = (logCount < LOG_BUFFER_SIZE) ? 0 : logIndex;
  for (int i = 0; i < logCount; i++) {
    int idx = (startIdx + i) % LOG_BUFFER_SIZE;

    const char* levelStr;
    switch (logBuffer[idx].level) {
      case DBG_LEVEL_ERROR:   levelStr = "ERR"; break;
      case DBG_LEVEL_WARN:    levelStr = "WARN"; break;
      case DBG_LEVEL_INFO:    levelStr = "INFO"; break;
      case DBG_LEVEL_VERBOSE: levelStr = "VERB"; break;
      default:                levelStr = "???"; break;
    }

    JsonObject log = logs.add<JsonObject>();
    log["t"] = logBuffer[idx].timestamp;
    log["l"] = levelStr;
    log["m"] = logBuffer[idx].message;
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// GET /api/timezones - Return list of all available timezones
void handleGetTimezones() {
  DBG_VERBOSE("GET /api/timezones\n");

  // Size: 102 timezones × (name(40) + tz(64)) = ~10,600 bytes. Use 12KB for safety.
  JsonDocument doc;
  JsonArray tzArray = doc.to<JsonArray>();

  for (int i = 0; i < numTimezones; i++) {
    JsonObject tz = tzArray.add<JsonObject>();
    tz["name"] = timezones[i].name;
    tz["tz"] = timezones[i].tzString;
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// Setup web server routes
void setupWebServer() {
  // IMPORTANT: Register API endpoints BEFORE static file handlers
  // WebServer processes routes in order - first match wins

  // API endpoints - GET requests
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/timezones", HTTP_GET, handleGetTimezones);
  server.on("/api/screenshot", HTTP_GET, handleScreenshot);
  server.on("/api/snapshot", HTTP_GET, handleSnapshot);
  server.on("/api/mirror", HTTP_GET, handleMirror);
  server.on("/api/debug", HTTP_GET, handleDebug);

  // Handle favicon.ico to prevent LittleFS errors
  server.on("/favicon.ico", HTTP_GET, []() {
    server.send(404);
  });

  // API endpoints - POST requests
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/debug-level", HTTP_POST, handleSetDebugLevel);
  server.on("/api/reset-wifi", HTTP_POST, handleResetWiFi);
  server.on("/api/reboot", HTTP_POST, handleReboot);

  // Serve static files from LittleFS (AFTER API routes to avoid conflicts)
  server.serveStatic("/app.js", LittleFS, "/app.js");
  server.serveStatic("/style.css", LittleFS, "/style.css");

  // Serve index.html from root (WebServer doesn't support setDefaultFile chaining)
  server.on("/", HTTP_GET, []() {
    if (!LittleFS.exists("/index.html")) {
      server.send(404, "text/plain", "index.html not found");
      return;
    }
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.begin();
  DBG_OK("Web server started on port 80");
}

// =========================
// Diagnostics Screen Rendering
// =========================

// Format uptime as HH:MM:SS
String formatUptime(unsigned long seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, minutes, secs);
  return String(buf);
}

// Draw full-screen diagnostics overlay
void drawDiagnosticsScreen() {
  tft.fillScreen(TFT_BLACK);

  // Force unload any smooth fonts and reset to bitmap font
  tft.unloadFont();
  tft.setTextFont(1);  // 6x8 pixel bitmap font
  tft.setTextSize(1);  // Normal size (not scaled)
  tft.setTextDatum(TL_DATUM);
  tft.setTextWrap(false);

  int y = 6;
  const int lineHeight = 10;

  // Title
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("=== DIAGNOSTICS ===", 10, y);
  y += lineHeight + 2;

  // System Info
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("SYSTEM:", 10, y);
  y += lineHeight;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("FW:" + String(FIRMWARE_VERSION) + " Heap:" + String(ESP.getFreeHeap() / 1024) + "K", 10, y);
  y += lineHeight;

  tft.drawString("Up:" + formatUptime(millis() / 1000) + " Dbg:" + String(debugLevel), 10, y);
  y += lineHeight + 2;

  // WiFi Info
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("NETWORK:", 10, y);
  y += lineHeight;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (WiFi.isConnected()) {
    String ssid = WiFi.SSID();
    if (ssid.length() > 20) ssid = ssid.substring(0, 20);
    tft.drawString("SSID: " + ssid, 10, y);
    y += lineHeight;

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("IP: " + WiFi.localIP().toString() + "  RSSI: " + String(WiFi.RSSI()) + "dBm", 10, y);
    y += lineHeight + 2;
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Not connected", 10, y);
    y += lineHeight + 2;
  }

  // Log entries
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("RECENT LOGS:", 10, y);
  y += lineHeight;

  // Calculate how many logs can fit
  int remainingHeight = tft.height() - y - 12;
  int maxLogs = remainingHeight / lineHeight;
  if (maxLogs > logCount) maxLogs = logCount;
  if (maxLogs > LOG_BUFFER_SIZE) maxLogs = LOG_BUFFER_SIZE;

  // Display most recent logs (circular buffer)
  for (int i = 0; i < maxLogs; i++) {
    int idx = (logIndex - maxLogs + i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    if (idx < 0) idx += LOG_BUFFER_SIZE;

    // Color by level
    uint16_t color;
    switch (logBuffer[idx].level) {
      case DBG_LEVEL_ERROR:   color = TFT_RED; break;
      case DBG_LEVEL_WARN:    color = TFT_YELLOW; break;
      case DBG_LEVEL_VERBOSE: color = TFT_DARKGREY; break;
      default:                color = TFT_WHITE; break;
    }

    tft.setTextColor(color, TFT_BLACK);

    // Format timestamp as MM:SS
    unsigned long secs = logBuffer[idx].timestamp / 1000;
    unsigned long mins = secs / 60;
    secs = secs % 60;
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", mins % 100, secs);

    // Truncate message to fit screen (wider with small font)
    String msg = String(logBuffer[idx].message);
    if (msg.length() > 32) {
      msg = msg.substring(0, 29) + "...";
    }

    String logLine = String(timeBuf) + " " + msg;
    tft.drawString(logLine, 10, y);
    y += lineHeight;
  }

  // Footer
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("Touch to dismiss (15s timeout)", tft.width() / 2, tft.height() - 2);
}

// =========================
// Touch Handling (XPT2046)
// =========================
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE = 500;  // 500ms debounce
bool lastTouchState = false;  // Track previous touch state for edge detection

// Check if screen is touched using IRQ pin (active LOW when touched)
// The touched() function always returns 1 on this board variant
bool isTouched() {
  // IRQ pin goes LOW when screen is touched
  return (digitalRead(XPT2046_IRQ) == LOW);
}

// Periodic touch status logging for debugging
static unsigned long lastTouchLog = 0;

void handleTouch() {
  bool currentTouchState = isTouched();

  // Debug: log touch state periodically
  if (millis() - lastTouchLog > 5000) {
    int irqState = digitalRead(XPT2046_IRQ);
    DBG_VERBOSE("Touch poll: IRQ=%d, current=%d, last=%d\n", irqState, currentTouchState, lastTouchState);
    lastTouchLog = millis();
  }

  // Only trigger on touch-down edge (was not touched, now is touched)
  if (!currentTouchState || lastTouchState) {
    lastTouchState = currentTouchState;
    return;
  }

  lastTouchState = currentTouchState;

  // Debounce
  unsigned long now = millis();
  if (now - lastTouchTime < TOUCH_DEBOUNCE) {
    DBG_VERBOSE("Touch debounced\n");
    return;
  }
  lastTouchTime = now;

  DBG_INFO("Touch detected!\n");

  // Toggle diagnostics screen
  showingDiagnostics = !showingDiagnostics;

  if (showingDiagnostics) {
    diagnosticsStartTime = now;
    drawDiagnosticsScreen();
    DBG_INFO("Diagnostics screen opened\n");
  } else {
    // Return to clock display
    // Properly unload any loaded font before resetting tracking
    if (currentSmoothFont != nullptr) {
      tft.unloadFont();
    }
    currentSmoothFont = nullptr;  // Reset font tracking so smooth fonts reload
    drawStaticLayout();
    lastDate[0] = '\0';  // Force redraw
    for (int i = 0; i < 6; i++) {
      lastTimes[i][0] = '\0';
      lastPrevDay[i] = false;
      lastColonState[i] = false;
    }
    // Reset analog clock state
    lastSecond = -1;
    lastMinute = -1;
    lastHour = -1;
    DBG_INFO("Diagnostics screen closed\n");
  }
}

// Check if diagnostics should auto-dismiss
void checkDiagnosticsTimeout() {
  if (!showingDiagnostics) return;

  if (millis() - diagnosticsStartTime > DIAGNOSTICS_TIMEOUT) {
    showingDiagnostics = false;
    // Properly unload any loaded font before resetting tracking
    if (currentSmoothFont != nullptr) {
      tft.unloadFont();
    }
    currentSmoothFont = nullptr;  // Reset font tracking so smooth fonts reload
    drawStaticLayout();
    lastDate[0] = '\0';  // Force redraw
    for (int i = 0; i < 6; i++) {
      lastTimes[i][0] = '\0';
      lastPrevDay[i] = false;
      lastColonState[i] = false;
    }
    // Reset analog clock state
    lastSecond = -1;
    lastMinute = -1;
    lastHour = -1;
    DBG_INFO("Diagnostics auto-closed\n");
  }
}

// =========================
// Startup Display & Splash Screen
// =========================

static int startupY = 10;
static const int startupLineHeight = 18;

void initStartupDisplay() {
  tft.init();
  tft.setRotation(0);  // Portrait for CYD
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  startupY = 10;

  // Title
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("CYD WORLD CLOCK", 10, startupY);
  startupY += startupLineHeight;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Firmware v" FIRMWARE_VERSION, 10, startupY);
  startupY += startupLineHeight + 4;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void showStartupStep(const char* msg, uint16_t color = TFT_WHITE) {
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(msg, 10, startupY);
  startupY += startupLineHeight;

  // Wrap around if we run out of space
  if (startupY > tft.height() - startupLineHeight) {
    startupY = 10;
    tft.fillScreen(TFT_BLACK);
  }
}

// Splash screen - simplified version for CYD World Clock
void showSplashScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  // Title display
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(4);
  tft.drawString("CYD WORLD CLOCK", tft.width() / 2, tft.height() / 2 - 20);

  // Version
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("v" FIRMWARE_VERSION, tft.width() / 2, tft.height() / 2 + 20);

  delay(1500);
  tft.fillScreen(TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);

  // Initialize LDR (Light Dependent Resistor) on ADC1
  pinMode(LDR_PIN, INPUT);
  analogSetAttenuation(ADC_11db);  // 0-3.3V range for full ADC reading
  delay(100);  // Allow ADC to stabilize
  DBG_INFO("LDR initialized on pin %d, initial reading: %d\n", LDR_PIN, readLDR());

  // Initialize touch screen with separate SPI bus (VSPI)
  // Using pin configuration confirmed working from fluid simulation demo
  pinMode(XPT2046_IRQ, INPUT);  // Configure IRQ pin as input
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchSPI);
  touchscreen.setRotation(0);  // Portrait mode to match display

  initStartupDisplay();
  DBG_INFO("CYD World Clock v%s starting...\n", FIRMWARE_VERSION);

  // Mount filesystem
  showStartupStep("Init LittleFS...");
  smoothFontsReady = LittleFS.begin(false);
  if (smoothFontsReady) {
    DBG_OK("LittleFS mounted");
    showStartupStep("LittleFS OK", TFT_GREEN);
    logLittleFSContents();
  } else {
    DBG_ERROR("LittleFS mount failed\n");
    showStartupStep("LittleFS FAIL", TFT_RED);
  }
  delay(300);

  // Load configuration
  showStartupStep("Loading config...");
  loadConfig();
  parseAllTimezones();  // Parse TZ strings for manual calculation (no setenv leak)
  DBG_OK("Config loaded");
  showStartupStep("Config OK", TFT_GREEN);
  delay(300);

  // WiFi
  showStartupStep("Connecting WiFi...");
  startWifi();
  if (WiFi.isConnected()) {
    showStartupStep(("IP: " + WiFi.localIP().toString()).c_str(), TFT_GREEN);
  } else {
    showStartupStep("WiFi: AP mode", TFT_YELLOW);
  }
  delay(500);

  // OTA
  showStartupStep("Init OTA...");
  setupOTA();
  showStartupStep("OTA ready", TFT_GREEN);
  delay(300);

  // Web server
  showStartupStep("Starting web...");
  setupWebServer();
  showStartupStep("Web server ready", TFT_GREEN);
  delay(300);

  // NTP sync
  showStartupStep("Syncing NTP...");
  if (!syncTime()) {
    DBG_ERROR("NTP sync failed\n");
    showStartupStep("NTP FAIL", TFT_RED);
    delay(3000);
    return;
  }
  DBG_OK("Time synced");
  showStartupStep("NTP synced", TFT_GREEN);
  delay(300);

  // Initialize touch
  showStartupStep("Init touch...");
  lastTouchState = isTouched();
  lastTouchTime = millis();  // Prevent immediate touch for debounce period
  int irqState = digitalRead(XPT2046_IRQ);
  DBG_INFO("Touch init: IRQ=%d, state=%d\n", irqState, lastTouchState);
  if (irqState == HIGH) {
    showStartupStep("Touch ready", TFT_GREEN);
  } else {
    showStartupStep("Touch (IRQ low)", TFT_YELLOW);
  }
  delay(300);

  // Apply display rotation based on config (portrait or landscape)
  // Do this before splash so splash shows in correct orientation
  applyRotation();

  // Splash screen (now in correct orientation)
  showSplashScreen();

  // Draw clock interface
  drawStaticLayout();

  // Show ready message
  DBG_INFO("==============================================\n");
  DBG_INFO("System ready! Touch screen to open diagnostics\n");
  DBG_INFO("==============================================\n");
}

// Track last display update time
static unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;  // Update display every 1 second

// Track last debug output time - 5 minutes to reduce overhead
static unsigned long lastDebugOutput = 0;
const unsigned long DEBUG_OUTPUT_INTERVAL = 300000;  // Output debug log every 5 minutes

void loop() {
  ArduinoOTA.handle();
  server.handleClient();  // Handle WebServer requests

  // Handle touch input (always, for responsiveness)
  handleTouch();
  checkDiagnosticsTimeout();

  // Skip clock updates when showing diagnostics
  if (showingDiagnostics) {
    delay(50);  // Fast polling for touch response
    return;
  }

  // Only update display once per second
  unsigned long now = millis();
  if (now - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) {
    delay(50);  // Short delay for touch responsiveness
    return;
  }
  lastDisplayUpdate = now;

  // Update clock display
  char dateStr[16];
  formatDate(dateStr, sizeof(dateStr));
  if (strcmp(dateStr, lastDate) != 0) {
    drawHeaderDate(dateStr);
    strlcpy(lastDate, dateStr, sizeof(lastDate));
  }
  drawTimes();

  // Display current times for all cities - compact format
  // Only output every 5 minutes to reduce overhead
  // Uses Serial.print directly to avoid heap allocation from String concatenation
  // Uses cached time values to avoid setenv() calls
  if (debugLevel >= DBG_LEVEL_INFO && (now - lastDebugOutput >= DEBUG_OUTPUT_INTERVAL)) {
    lastDebugOutput = now;

    // Ensure cache is up to date (uses batch update if minute changed)
    formatTime(0);

    // Build compact single-line output using Serial.print to avoid String heap allocation
    Serial.print("[INFO] ");

    // Home city (with HOME indicator) - use cached values directly
    Serial.print(config.homeCityLabel);
    Serial.print(" (HOME) ");
    Serial.print(timeCache[0].timeStr);

    // Remote cities - use cached values directly
    for (int i = 0; i < 5; i++) {
      Serial.print(" | ");
      Serial.print(config.remoteCities[i]);
      Serial.print(" ");
      Serial.print(timeCache[i + 1].timeStr);
      if (timeCache[i + 1].prevDay) {
        Serial.print(" (PREV DAY)");
      }
    }

    // Also report heap and LDR status
    Serial.print(" | Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" bytes | LDR: ");
    Serial.print(readLDR());
    Serial.println();

    // Warn if heap is getting low
    if (ESP.getFreeHeap() < 20000) {
      DBG_WARN("Low heap: %u bytes free\n", ESP.getFreeHeap());
    }
  }
}

// =========================
// Screenshot Functionality
// =========================
/**
 * Capture screenshot from TFT display and output as PPM image via serial
 *
 * USAGE:
 *   1. Call takeScreenshot() from serial command or web endpoint
 *   2. Capture serial output to file: screenshot.ppm
 *   3. Convert with ImageMagick: convert screenshot.ppm screenshot.png
 *
 * Format: PPM (Portable Pixmap) - simple uncompressed RGB format
 * Output size: ~230KB for 240x320 display
 */
void takeScreenshot() {
  DBG_INFO("Taking screenshot...\n");

  // PPM header: P6 = binary RGB, width height, max color value
  Serial.println("P6");
  Serial.printf("%d %d\n", tft.width(), tft.height());
  Serial.println("255");

  // Read pixel by pixel and output RGB values
  for (int y = 0; y < tft.height(); y++) {
    for (int x = 0; x < tft.width(); x++) {
      // Read pixel as RGB565
      uint16_t color565 = tft.readPixel(x, y);

      // Convert RGB565 to RGB888
      uint8_t r = ((color565 >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
      uint8_t g = ((color565 >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits
      uint8_t b = (color565 & 0x1F) << 3;          // 5 bits -> 8 bits

      // Output RGB bytes
      Serial.write(r);
      Serial.write(g);
      Serial.write(b);
    }

    // Progress indicator every 32 lines (to stderr so it doesn't corrupt image)
    if (y % 32 == 0) {
      DBG_INFO("Screenshot progress: %d%%\n", (y * 100) / tft.height());
    }
  }

  DBG_INFO("Screenshot complete!\n");
}

/**
 * Alternative: Output as raw RGB565 data (smaller but requires conversion)
 * Outputs binary RGB565 format - each pixel is 2 bytes
 */
void takeScreenshotRaw() {
  DBG_INFO("Taking raw screenshot (RGB565)...\n");
  Serial.println("SCREENSHOT_START");
  Serial.printf("WIDTH:%d\n", tft.width());
  Serial.printf("HEIGHT:%d\n", tft.height());
  Serial.println("DATA:");

  for (int y = 0; y < tft.height(); y++) {
    for (int x = 0; x < tft.width(); x++) {
      uint16_t color = tft.readPixel(x, y);
      Serial.write((color >> 8) & 0xFF);  // High byte
      Serial.write(color & 0xFF);         // Low byte
    }
  }

  Serial.println("\nSCREENSHOT_END");
  DBG_INFO("Raw screenshot complete!\n");
}
