// CYD World Clock: ESP32 + ILI9341 world clock display with WiFiManager + NTP.
#include <Arduino.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <time.h>
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

// Conditional debug macros based on debug level
#define DBG_ERROR(...)   do { if (debugLevel >= DBG_LEVEL_ERROR) { Serial.print("[ERR ] "); Serial.printf(__VA_ARGS__); } } while(0)
#define DBG_WARN(...)    do { if (debugLevel >= DBG_LEVEL_WARN) { Serial.print("[WARN] "); Serial.printf(__VA_ARGS__); } } while(0)
#define DBG_INFO(...)    do { if (debugLevel >= DBG_LEVEL_INFO) { Serial.print("[INFO] "); Serial.printf(__VA_ARGS__); } } while(0)
#define DBG_VERBOSE(...) do { if (debugLevel >= DBG_LEVEL_VERBOSE) { Serial.print("[VERB] "); Serial.printf(__VA_ARGS__); } } while(0)

// Legacy compatibility macros
#define DBG(...)      DBG_INFO(__VA_ARGS__)
#define DBGLN(s)      DBG_INFO("%s\n", s)
#define DBG_STEP(s)   DBG_INFO("%s\n", s)
#define DBG_OK(s)     DBG_INFO("✓ %s\n", s)
#define DBG_ERR(s)    DBG_ERROR("%s\n", s)

// =========================
// Global Objects & Configuration
// =========================
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Preferences prefs;

#define FIRMWARE_VERSION "2.0.0"
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

// Cached state to minimize redraws and flicker.
String lastDate;
String lastTimes[6];  // 6 cities: home + 5 remote
bool lastPrevDay[6];
bool lastColonState[6];
int timePadWidth = 0;
unsigned long lastDebugPrint = 0;
bool smoothFontsReady = false;
const char *currentSmoothFont = nullptr;

void setTimezone(const char *tz) {
  setenv("TZ", tz, 1);
  tzset();
}

// Switch between smooth fonts (loaded from SPIFFS) or fallback bitmap fonts.
void setFont(const char *smoothFontName, int fallbackFont) {
  if (kUseSmoothFonts && smoothFontsReady) {
    if (currentSmoothFont != smoothFontName) {
      if (currentSmoothFont) {
        tft.unloadFont();
      }
      String path = "/";
      path += smoothFontName;
      path += ".vlw";
      if (LittleFS.exists(path)) {
        tft.loadFont(smoothFontName, LittleFS);
        currentSmoothFont = smoothFontName;
        return;
      }
      currentSmoothFont = nullptr;
    }
  }
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

// Return a date string for the given timezone (e.g., "THU 24 MAR").
String formatDate(const char *tz) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  setTimezone(tz);
  localtime_r(&now, &timeinfo);
  char buf[16];
  strftime(buf, sizeof(buf), "%a %d %b", &timeinfo);
  String dateStr(buf);
  dateStr.toUpperCase();
  return dateStr;
}

// Helper to get a local time struct for an arbitrary TZ and timestamp.
void getLocalTm(const char *tz, time_t now, struct tm *out) {
  setTimezone(tz);
  localtime_r(&now, out);
}

struct TimeInfo {
  String timeStr;
  bool prevDay;
  bool showColon;
};

// Build time string plus flags used by the renderer.
// - timeStr is always "HH:MM" so the width stays stable.
// - showColon toggles each second for blink.
// - prevDay is based on comparison with home city's date.
TimeInfo formatTime(const char *tz, const struct tm &homeTm) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  getLocalTm(tz, now, &timeinfo);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  bool prevDay = false;
  if (timeinfo.tm_year < homeTm.tm_year) {
    prevDay = true;
  } else if (timeinfo.tm_year == homeTm.tm_year &&
             timeinfo.tm_yday < homeTm.tm_yday) {
    prevDay = true;
  }

  TimeInfo info;
  info.timeStr = String(buf);
  info.prevDay = prevDay;
  info.showColon = (timeinfo.tm_sec % 2 == 0);
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
void drawHeaderDate(const String &dateStr) {
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
    TimeInfo info = formatTime(getTzByIndex(i), homeTm);
    bool timeChanged = (info.timeStr != lastTimes[i]);
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
    lastTimes[i] = info.timeStr;
    lastPrevDay[i] = info.prevDay;
    lastColonState[i] = info.showColon;
  }
}

// =========================
// WiFi & OTA Setup
// =========================

static void configModeCallback(WiFiManager* myWiFiManager) {
  DBG_INFO("Entered WiFi config mode\n");
  DBG_INFO("Connect to AP: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  DBG_INFO("Config portal IP: %s\n", WiFi.softAPIP().toString().c_str());
}

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
  }

  if (WiFi.isConnected()) {
    DBG_INFO("WiFi connected: SSID=%s IP=%s\n",
        WiFi.SSID().c_str(),
        WiFi.localIP().toString().c_str());
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
    tft.drawString("UPDATE COMPLETE", tft.width() / 2, tft.height() / 2);
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

// GET /api/state - Return current configuration as JSON
void handleGetState() {
  String json = "{";
  json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"hostname\":\"" + String(OTA_HOSTNAME) + "\",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"wifi_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"homeCity\":{";
  json += "\"label\":\"" + String(config.homeCityLabel) + "\",";
  json += "\"tz\":\"" + String(config.homeCityTz) + "\"";
  json += "},\"remoteCities\":[";
  for (int i = 0; i < 5; i++) {
    json += "{\"label\":\"" + String(config.remoteCities[i]) + "\",";
    json += "\"tz\":\"" + String(config.remoteTzStrings[i]) + "\"}";
    if (i < 4) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
  DBG_INFO("GET /api/state\n");
}

// POST /api/config - Update configuration
void handlePostConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing body");
    return;
  }

  String body = server.arg("plain");
  DBG_INFO("POST /api/config: %s\n", body.c_str());

  // Simple JSON parsing for our known structure
  // Expected: {"homeCity":{"label":"X","tz":"Y"},"remoteCities":[{...},{...},{...},{...}]}

  // Parse home city
  int homeLabelStart = body.indexOf("\"homeCity\":{\"label\":\"") + 21;
  int homeLabelEnd = body.indexOf("\"", homeLabelStart);
  if (homeLabelStart > 21 && homeLabelEnd > homeLabelStart) {
    String homeLabel = body.substring(homeLabelStart, homeLabelEnd);
    DBG_INFO("POST homeLabel='%s'\n", homeLabel.c_str());
    String cityOnly = extractCityName(homeLabel);
    DBG_INFO("POST cityOnly='%s'\n", cityOnly.c_str());
    strlcpy(config.homeCityLabel, cityOnly.c_str(), sizeof(config.homeCityLabel));
    DBG_INFO("POST config.homeCityLabel='%s'\n", config.homeCityLabel);
  }

  int homeTzStart = body.indexOf("\"tz\":\"", homeLabelEnd) + 6;
  int homeTzEnd = body.indexOf("\"", homeTzStart);
  if (homeTzStart > 6 && homeTzEnd > homeTzStart) {
    String homeTz = body.substring(homeTzStart, homeTzEnd);
    strlcpy(config.homeCityTz, homeTz.c_str(), sizeof(config.homeCityTz));
  }

  // Parse remote cities (simple extraction, assumes order)
  int searchPos = body.indexOf("\"remoteCities\":[") + 16;
  for (int i = 0; i < 5 && searchPos > 16; i++) {
    int labelStart = body.indexOf("\"label\":\"", searchPos) + 9;
    int labelEnd = body.indexOf("\"", labelStart);
    if (labelStart > 9 && labelEnd > labelStart) {
      String label = body.substring(labelStart, labelEnd);
      String cityOnly = extractCityName(label);
      strlcpy(config.remoteCities[i], cityOnly.c_str(), sizeof(config.remoteCities[i]));
    }

    int tzStart = body.indexOf("\"tz\":\"", labelEnd) + 6;
    int tzEnd = body.indexOf("\"", tzStart);
    if (tzStart > 6 && tzEnd > tzStart) {
      String tz = body.substring(tzStart, tzEnd);
      strlcpy(config.remoteTzStrings[i], tz.c_str(), sizeof(config.remoteTzStrings[i]));
    }

    searchPos = tzEnd + 1;
  }

  saveConfig();
  loadConfig();  // Reload config immediately - no reboot needed

  // Redraw static layout with new city labels
  drawStaticLayout();

  // Reset cached state to force full redraw of times on next loop
  lastDate = "";
  for (int i = 0; i < 6; i++) {
    lastTimes[i] = "";
    lastPrevDay[i] = false;
    lastColonState[i] = false;
  }

  server.send(200, "text/plain", "OK - Config updated");
  DBG_INFO("Config updated and reloaded\n");
}

// POST /api/reset-wifi - Clear WiFi credentials
void handleResetWiFi() {
  DBG_INFO("POST /api/reset-wifi\n");
  WiFiManager wm;
  wm.resetSettings();
  server.send(200, "text/plain", "WiFi reset. Rebooting...");
  delay(1000);
  ESP.restart();
}

void handleReboot() {
  DBG_INFO("POST /api/reboot\n");
  server.send(200, "text/plain", "Rebooting device...");
  delay(1000);
  ESP.restart();
}

// GET /api/timezones - Return list of all available timezones
void handleGetTimezones() {
  DBG_INFO("GET /api/timezones\n");
  String json = "[";
  for (int i = 0; i < numTimezones; i++) {
    json += "{\"name\":\"" + String(timezones[i].name) + "\",";
    json += "\"tz\":\"" + String(timezones[i].tzString) + "\"}";
    if (i < numTimezones - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Setup web server routes
void setupWebServer() {
  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/index.html");
  server.serveStatic("/app.js", LittleFS, "/app.js");
  server.serveStatic("/style.css", LittleFS, "/style.css");

  // API endpoints
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/timezones", HTTP_GET, handleGetTimezones);
  server.on("/api/reset-wifi", HTTP_POST, handleResetWiFi);
  server.on("/api/reboot", HTTP_POST, handleReboot);

  server.begin();
  DBG_OK("Web server started on port 80");
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
  if (WiFi.isConnected()) {
    showStartupStep(("Web: " + WiFi.localIP().toString()).c_str(), TFT_GREEN);
  } else {
    showStartupStep("Web: AP mode", TFT_YELLOW);
  }
  delay(500);

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
  delay(800);

  // Splash screen
  showSplashScreen();

  // Draw clock interface
  drawStaticLayout();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Update header date and per-location times once per second.
  String dateStr = formatDate(config.homeCityTz);
  if (dateStr != lastDate) {
    drawHeaderDate(dateStr);
    lastDate = dateStr;
  }
  drawTimes();

  // Display current times for all cities (debug level 3)
  if (debugLevel >= DBG_LEVEL_INFO) {
    time_t now = time(nullptr);
    struct tm homeTm;
    getLocalTm(config.homeCityTz, now, &homeTm);

    // Print home city time
    TimeInfo homeInfo = formatTime(config.homeCityTz, homeTm);
    DBG_INFO("Home %-18s %s\n", config.homeCityLabel, homeInfo.timeStr.c_str());

    // Print remote city times
    for (int i = 0; i < 5; i++) {
      TimeInfo remoteInfo = formatTime(config.remoteTzStrings[i], homeTm);
      const char* dayIndicator = remoteInfo.prevDay ? " (PREV DAY)" : "";
      DBG_INFO("     %-18s %s%s\n", config.remoteCities[i], remoteInfo.timeStr.c_str(), dayIndicator);
    }
    DBG_INFO("---\n");
  }

  delay(1000);
}
