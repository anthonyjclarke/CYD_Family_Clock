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

// Helper to format and add to log buffer
void logToBuffer(uint8_t level, const char* format, ...) {
  char buffer[80];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  addToLogBuffer(level, buffer);
}

// Conditional debug macros based on debug level - now also log to buffer
// Buffer size matches log buffer entry size (80 bytes) to avoid memory waste
#define DBG_ERROR(...)   do { if (debugLevel >= DBG_LEVEL_ERROR) { Serial.print("[ERR ] "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_ERROR, buf); } } while(0)
#define DBG_WARN(...)    do { if (debugLevel >= DBG_LEVEL_WARN) { Serial.print("[WARN] "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_WARN, buf); } } while(0)
#define DBG_INFO(...)    do { if (debugLevel >= DBG_LEVEL_INFO) { Serial.print("[INFO] "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_INFO, buf); } } while(0)
#define DBG_VERBOSE(...) do { if (debugLevel >= DBG_LEVEL_VERBOSE) { Serial.print("[VERB] "); Serial.printf(__VA_ARGS__); char buf[80]; snprintf(buf, sizeof(buf), __VA_ARGS__); addToLogBuffer(DBG_LEVEL_VERBOSE, buf); } } while(0)

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

#define FIRMWARE_VERSION "2.2.0"
#define OTA_HOSTNAME "CYD-WorldClock"
#define OTA_PASSWORD "change-me"  // TODO: Change this!

// Configuration structure
struct Config {
  char homeCityLabel[32];
  char homeCityTz[64];
  char remoteCities[5][32];    // City labels (expanded to 5)
  char remoteTzStrings[5][64]; // Timezone strings (expanded to 5)
};

Config config;

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

// Helper: Extract city name only (strip country after comma)
String extractCityName(const String& fullLabel) {
  int commaPos = fullLabel.indexOf(',');
  if (commaPos > 0) {
    return fullLabel.substring(0, commaPos);
  }
  return fullLabel;
}

// Load configuration from NVS
void loadConfig() {
  prefs.begin(PREF_NAMESPACE, false);

  // Load home city
  String homeLabel = prefs.getString(PREF_HOME_LABEL, DEFAULT_HOME_LABEL);
  String homeTz = prefs.getString(PREF_HOME_TZ, DEFAULT_HOME_TZ);
  DBG_INFO("NVS read homeLabel='%s'\n", homeLabel.c_str());
  String homeCityOnly = extractCityName(homeLabel);
  DBG_INFO("After extract: homeCityOnly='%s'\n", homeCityOnly.c_str());
  strlcpy(config.homeCityLabel, homeCityOnly.c_str(), sizeof(config.homeCityLabel));
  DBG_INFO("After strlcpy: config.homeCityLabel='%s'\n", config.homeCityLabel);
  strlcpy(config.homeCityTz, homeTz.c_str(), sizeof(config.homeCityTz));

  // Load 5 remote cities
  for (int i = 0; i < 5; i++) {
    String labelKey = String(PREF_REMOTE_PREFIX) + i + "Label";
    String tzKey = String(PREF_REMOTE_PREFIX) + i + "Tz";
    String label = prefs.getString(labelKey.c_str(), DEFAULT_REMOTE_LABELS[i]);
    String tz = prefs.getString(tzKey.c_str(), DEFAULT_REMOTE_TZS[i]);
    String cityOnly = extractCityName(label);
    strlcpy(config.remoteCities[i], cityOnly.c_str(), sizeof(config.remoteCities[i]));
    strlcpy(config.remoteTzStrings[i], tz.c_str(), sizeof(config.remoteTzStrings[i]));
  }

  prefs.end();
  DBG_INFO("Config loaded: Home=%s, Remote0=%s\n", config.homeCityLabel, config.remoteCities[0]);
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
  prefs.end();
  DBG_INFO("Config saved\n");
}

// Color palette.
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_LABEL = TFT_WHITE;
static const uint16_t COLOR_TIME = TFT_GREEN;

// Layout + font settings (easy to tweak for readability).
const int kTitleHeight = 22;   // "WORLD CLOCK" title row
const int kDateHeight = 18;    // Date display row
const int kHeaderHeight = kTitleHeight + kDateHeight;  // Total header = 40px
const int kPad = 8;
const int kBacklightPin = 21;
const bool kUseSmoothFonts = true;
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
bool lastColonState[6];
int timePadWidth = 0;
unsigned long lastDebugPrint = 0;
bool smoothFontsReady = false;
const char *currentSmoothFont = nullptr;

// CRITICAL FIX: setenv("TZ") leaks memory on ESP32 when called repeatedly
// Solution: Cache the last timezone set to minimize setenv() calls
static const char* lastSetTimezone = nullptr;

void getLocalTm(const char *tz, time_t now, struct tm *out) {
  // Only call setenv/tzset if timezone actually changed
  // Use pointer comparison first (works for string literals), then strcmp for safety
  if (lastSetTimezone != tz && (lastSetTimezone == nullptr || strcmp(lastSetTimezone, tz) != 0)) {
    setenv("TZ", tz, 1);
    tzset();
    lastSetTimezone = tz;
  }

  localtime_r(&now, out);
}

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

// Format a date string for the given timezone (e.g., "THU 24 MAR").
// Writes directly to provided buffer to avoid heap allocation.
void formatDate(const char *tz, char *outBuf, size_t bufSize) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  getLocalTm(tz, now, &timeinfo);
  strftime(outBuf, bufSize, "%a %d %b", &timeinfo);
  // Convert to uppercase in place
  for (char *p = outBuf; *p; ++p) {
    *p = toupper((unsigned char)*p);
  }
}

struct TimeInfo {
  char timeStr[8];  // "HH:MM" + null terminator - fixed size to avoid heap allocation
  bool prevDay;
  bool showColon;
};

// CRITICAL FIX: Cache formatted times to avoid repeated setenv() calls
// Only recalculate when the minute changes
struct CachedTimeInfo {
  struct tm tm;
  char timeStr[8];
  bool prevDay;
  time_t lastCalculated;
};

static CachedTimeInfo timeCache[6];  // Cache for all 6 cities (home + 5 remote)
static bool timeCacheInitialized = false;

// Build time string plus flags used by the renderer.
// - timeStr is always "HH:MM" so the width stays stable.
// - showColon toggles each second for blink.
// - prevDay is based on comparison with home city's date.
TimeInfo formatTime(const char *tz, const struct tm &homeTm, int cityIndex) {
  time_t now = time(nullptr);
  TimeInfo info;

  // Check if we need to recalculate (minute changed or first call for this city)
  if (timeCache[cityIndex].lastCalculated == 0 || (now / 60) != (timeCache[cityIndex].lastCalculated / 60)) {
    // Recalculate timezone data (only happens once per minute per city)
    getLocalTm(tz, now, &timeCache[cityIndex].tm);
    snprintf(timeCache[cityIndex].timeStr, sizeof(timeCache[cityIndex].timeStr),
             "%02d:%02d", timeCache[cityIndex].tm.tm_hour, timeCache[cityIndex].tm.tm_min);

    timeCache[cityIndex].prevDay = false;
    if (timeCache[cityIndex].tm.tm_year < homeTm.tm_year) {
      timeCache[cityIndex].prevDay = true;
    } else if (timeCache[cityIndex].tm.tm_year == homeTm.tm_year &&
               timeCache[cityIndex].tm.tm_yday < homeTm.tm_yday) {
      timeCache[cityIndex].prevDay = true;
    }

    timeCache[cityIndex].lastCalculated = now;
    timeCacheInitialized = true;  // Mark cache as initialized
  }

  // Use cached values
  strcpy(info.timeStr, timeCache[cityIndex].timeStr);
  info.prevDay = timeCache[cityIndex].prevDay;
  info.showColon = (now % 2 == 0);  // Blink every second

  return info;
}

// Draw the static header and location labels once.
void drawStaticLayout() {
  tft.fillScreen(COLOR_BG);

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

// Draw or update the header date string.
void drawHeaderDate(const char *dateStr) {
  setFont(kFontHeader, kFallbackHeader);
  tft.setTextColor(COLOR_TIME, COLOR_BG);
  tft.setTextDatum(MC_DATUM);
  // Clear date row (below title) and draw centered date
  tft.fillRect(0, kTitleHeight, tft.width(), kDateHeight, COLOR_BG);
  tft.drawString(dateStr, tft.width() / 2, kTitleHeight + kDateHeight / 2 + 2);
}

// Draw times for each location and update only when needed.
void drawTimes() {
  int rows = 6;  // Home + 5 remote cities
  int rowHeight = (tft.height() - kHeaderHeight) / rows;

  time_t now = time(nullptr);
  struct tm homeTm;
  getLocalTm(config.homeCityTz, now, &homeTm);

  setFont(kFontTime, kFallbackTime);
  if (timePadWidth == 0) {
    // Reserve a fixed width so the time does not "move".
    timePadWidth = tft.textWidth("88:88");
  }
  tft.setTextPadding(timePadWidth);
  tft.setTextColor(COLOR_TIME, COLOR_BG);
  tft.setTextDatum(TR_DATUM);

  // Helper to get timezone string by index
  auto getTzByIndex = [](int i) -> const char* {
    if (i == 0) return config.homeCityTz;
    return config.remoteTzStrings[i - 1];
  };

  // Helper to get city label by index
  auto getLabelByIndex = [](int i) -> const char* {
    if (i == 0) return config.homeCityLabel;
    return config.remoteCities[i - 1];
  };

  for (int i = 0; i < rows; ++i) {
    tft.setTextPadding(timePadWidth);
    TimeInfo info = formatTime(getTzByIndex(i), homeTm, i);
    bool timeChanged = (strcmp(info.timeStr, lastTimes[i]) != 0);
    bool prevDayChanged = (info.prevDay != lastPrevDay[i]);
    bool colonChanged = (info.showColon != lastColonState[i]);
    if (!timeChanged && !prevDayChanged && !colonChanged) {
      continue;
    }

    int rowTop = kHeaderHeight + i * rowHeight;
    int timeY = rowTop + 2;
    int widthMin = tft.textWidth("00");
    int widthColon = tft.textWidth(":");
    int colonX = (tft.width() - kPad) - widthMin - widthColon;

    if (timeChanged || prevDayChanged) {
      // Clear only the right side where time is displayed, leaving city labels intact
      // Time is "HH:MM" (5 chars), calculate actual start position
      setFont(kFontTime, kFallbackTime);
      int timeWidth = tft.textWidth("88:88");  // Max width for time
      int clearX = (tft.width() - kPad) - timeWidth - 2;  // Start 2px before time begins
      tft.fillRect(clearX, rowTop, tft.width() - clearX, rowHeight, COLOR_BG);
      tft.setTextDatum(TR_DATUM);
      tft.drawString(info.timeStr, tft.width() - kPad, timeY);
    }

    if (colonChanged) {
      // Only touch the colon region to avoid flicker.
      tft.setTextPadding(0);
      tft.setTextDatum(TL_DATUM);
      if (info.showColon) {
        tft.setTextColor(COLOR_TIME, COLOR_BG);
      } else {
        tft.setTextColor(COLOR_BG, COLOR_BG);
      }
      tft.drawString(":", colonX, timeY);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(COLOR_TIME, COLOR_BG);
    }

    if (prevDayChanged) {
      // Clear left side for city label and "PREV DAY" note
      int labelClearWidth = tft.width() / 2 - kPad - 4;  // Extra margin to prevent clipping
      tft.fillRect(kPad, rowTop, labelClearWidth, rowHeight, COLOR_BG);
      setFont(kFontLabel, kFallbackLabel);
      tft.setTextColor(COLOR_LABEL, COLOR_BG);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(getLabelByIndex(i), kPad, rowTop + 2);
      if (info.prevDay) {
        setFont(kFontNote, kFallbackNote);
        tft.setTextColor(TFT_YELLOW, COLOR_BG);
        tft.drawString("PREV DAY", kPad, rowTop + 2 + tft.fontHeight() + 2);
      }
    }

    setFont(kFontTime, kFallbackTime);
    tft.setTextColor(COLOR_TIME, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    strlcpy(lastTimes[i], info.timeStr, sizeof(lastTimes[i]));
    lastPrevDay[i] = info.prevDay;
    lastColonState[i] = info.showColon;
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
  tft.drawString("Timeout: 3 minutes", tft.width() / 2, tft.height() - 10);
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
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);
  wm.setAPCallback(configModeCallback);

  bool ok = wm.autoConnect("CYD-WorldClock-Setup");
  if (!ok) {
    DBG_WARN("WiFiManager autoConnect failed/timeout. Starting fallback AP...\n");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("CYD-WorldClock-AP");
    // Display fallback AP instructions
    displayWiFiSetupInstructions("CYD-WorldClock-AP", WiFi.softAPIP().toString().c_str());
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
      String cityOnly = extractCityName(String(homeLabel));
      strlcpy(config.homeCityLabel, cityOnly.c_str(), sizeof(config.homeCityLabel));
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
        String cityOnly = extractCityName(String(label));
        strlcpy(config.remoteCities[i], cityOnly.c_str(), sizeof(config.remoteCities[i]));
      }
      if (!city["tz"].isNull()) {
        const char* tz = city["tz"];
        strlcpy(config.remoteTzStrings[i], tz, sizeof(config.remoteTzStrings[i]));
      }
      i++;
    }
  }

  saveConfig();
  loadConfig();  // Reload config immediately - no reboot needed

  // Redraw static layout with new city labels
  drawStaticLayout();

  // Reset cached state to force full redraw of times on next loop
  lastDate[0] = '\0';
  for (int i = 0; i < 6; i++) {
    lastTimes[i][0] = '\0';
    lastPrevDay[i] = false;
    lastColonState[i] = false;
  }

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

// GET /api/screenshot - Trigger screenshot capture
void handleScreenshot() {
  DBG_INFO("GET /api/screenshot\n");
  server.send(200, "text/plain", "Screenshot will be sent via serial. Monitor serial output.");
  delay(500);  // Give web response time to send
  takeScreenshot();
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

  // Phase 1: Title fade-in
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(4);
  for (int alpha = 0; alpha < 5; alpha++) {
    tft.drawString("CYD WORLD CLOCK", tft.width() / 2, tft.height() / 2 - 30);
    delay(100);
  }
  delay(800);

  // Phase 2: Globe/timezone animation (simple circle with dots)
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;
  int radius = 50;

  // Draw "globe" outline
  tft.drawCircle(cx, cy, radius, TFT_GREEN);
  delay(200);

  // Draw timezone markers around globe
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30) * PI / 180.0;
    int x = cx + radius * cos(angle);
    int y = cy + radius * sin(angle);
    tft.fillCircle(x, y, 3, TFT_YELLOW);
    delay(80);
  }
  delay(500);

  tft.fillScreen(TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);

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

  // Splash screen
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
  formatDate(config.homeCityTz, dateStr, sizeof(dateStr));
  if (strcmp(dateStr, lastDate) != 0) {
    drawHeaderDate(dateStr);
    strlcpy(lastDate, dateStr, sizeof(lastDate));
  }
  drawTimes();

  // Display current times for all cities - compact format
  // Only output every 5 minutes to reduce overhead
  // Uses Serial.print directly to avoid heap allocation from String concatenation
  if (debugLevel >= DBG_LEVEL_INFO && (now - lastDebugOutput >= DEBUG_OUTPUT_INTERVAL)) {
    lastDebugOutput = now;

    time_t nowTime = time(nullptr);
    struct tm homeTm;
    getLocalTm(config.homeCityTz, nowTime, &homeTm);

    // Build compact single-line output using Serial.print to avoid String heap allocation
    Serial.print("[INFO] ");

    // Home city (with HOME indicator)
    TimeInfo homeInfo = formatTime(config.homeCityTz, homeTm, 0);
    Serial.print(config.homeCityLabel);
    Serial.print(" (HOME) ");
    Serial.print(homeInfo.timeStr);

    // Remote cities
    for (int i = 0; i < 5; i++) {
      TimeInfo remoteInfo = formatTime(config.remoteTzStrings[i], homeTm, i + 1);
      Serial.print(" | ");
      Serial.print(config.remoteCities[i]);
      Serial.print(" ");
      Serial.print(remoteInfo.timeStr);
      if (remoteInfo.prevDay) {
        Serial.print(" (PREV DAY)");
      }
    }

    // Also report heap status
    Serial.print(" | Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

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
