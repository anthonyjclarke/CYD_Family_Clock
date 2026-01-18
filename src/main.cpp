// CYD World Clock: ESP32 + ILI9341 world clock display with WiFiManager + NTP.
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>

// Enable serial logging for startup and periodic time output.
const bool DEBUG_LOGS = true;

// Sydney is the reference day for "previous day" notation and NTP sync.
const char *SYDNEY_TZ = "AEST-10AEDT,M10.1.0/2,M4.1.0/3";

struct Location {
  const char *label;
  const char *tz;
};

// Display order (top-to-bottom).
Location locations[] = {
    {"SYDNEY", SYDNEY_TZ},
    {"VANCOUVER", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {"LONDON", "GMT0BST,M3.5.0/1,M10.5.0/2"},
    {"NAIROBI", "EAT-3"},
    {"DENVER", "MST7MDT,M3.2.0/2,M11.1.0/2"},
};

TFT_eSPI tft = TFT_eSPI();

// Color palette.
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_LABEL = TFT_WHITE;
static const uint16_t COLOR_TIME = TFT_GREEN;

// Layout + font settings (easy to tweak for readability).
const int kHeaderHeight = 28;
const int kPad = 8;
const int kBacklightPin = 21;
const int kFontHeader = 2;
const int kFontLabel = 4;
const int kFontTime = 6;
const int kFontNote = 2;

// Cached state to minimize redraws and flicker.
String lastDate;
String lastTimes[sizeof(locations) / sizeof(locations[0])];
bool lastPrevDay[sizeof(locations) / sizeof(locations[0])];
bool lastColonState[sizeof(locations) / sizeof(locations[0])];
int timePadWidth = 0;
unsigned long lastDebugPrint = 0;

void setTimezone(const char *tz) {
  setenv("TZ", tz, 1);
  tzset();
}

// Block until NTP has set a valid time (returns false on timeout).
bool syncTime() {
  configTzTime(SYDNEY_TZ, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  for (int i = 0; i < 20; ++i) {
    if (getLocalTime(&timeinfo)) {
      return true;
    }
    delay(10);
    yield();
    delay(500);
  }
  return false;
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
// - prevDay is based on comparison with Sydney's date.
TimeInfo formatTime(const char *tz, const struct tm &sydneyTm) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  getLocalTm(tz, now, &timeinfo);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  bool prevDay = false;
  if (timeinfo.tm_year < sydneyTm.tm_year) {
    prevDay = true;
  } else if (timeinfo.tm_year == sydneyTm.tm_year &&
             timeinfo.tm_yday < sydneyTm.tm_yday) {
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
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  tft.setTextFont(kFontHeader);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("WORLD CLOCK", kPad, 6);

  int rows = sizeof(locations) / sizeof(locations[0]);
  int rowHeight = (tft.height() - kHeaderHeight) / rows;

  for (int i = 0; i < rows; ++i) {
    int rowTop = kHeaderHeight + i * rowHeight;
    tft.setTextFont(kFontLabel);
    int labelY = rowTop + 2;
    tft.setTextDatum(TL_DATUM);
    tft.drawString(locations[i].label, kPad, labelY);
  }
}

// Draw or update the header date string.
void drawHeaderDate(const String &dateStr) {
  tft.setTextFont(kFontHeader);
  tft.setTextColor(COLOR_TIME, COLOR_BG);
  tft.setTextDatum(TR_DATUM);
  tft.fillRect(120, 0, tft.width() - 120, kHeaderHeight, COLOR_BG);
  tft.drawString(dateStr, tft.width() - kPad, 6);
}

// Draw times for each location and update only when needed.
void drawTimes() {
  int rows = sizeof(locations) / sizeof(locations[0]);
  int rowHeight = (tft.height() - kHeaderHeight) / rows;

  time_t now = time(nullptr);
  struct tm sydneyTm;
  getLocalTm(SYDNEY_TZ, now, &sydneyTm);

  tft.setTextFont(kFontTime);
  if (timePadWidth == 0) {
    // Reserve a fixed width so the time does not "move".
    timePadWidth = tft.textWidth("88:88");
  }
  tft.setTextPadding(timePadWidth);
  tft.setTextColor(COLOR_TIME, COLOR_BG);
  tft.setTextDatum(TR_DATUM);

  for (int i = 0; i < rows; ++i) {
    tft.setTextPadding(timePadWidth);
    TimeInfo info = formatTime(locations[i].tz, sydneyTm);
    bool changed = (info.timeStr != lastTimes[i]) ||
                   (info.prevDay != lastPrevDay[i]) ||
                   (info.showColon != lastColonState[i]);
    if (!changed) {
      continue;
    }

    int rowTop = kHeaderHeight + i * rowHeight;
    int clearX = tft.width() / 2;
    tft.fillRect(clearX, rowTop, tft.width() - clearX, rowHeight, COLOR_BG);

    int timeY = rowTop + 2;
    tft.drawString(info.timeStr, tft.width() - kPad, timeY);
    if (!info.showColon) {
      // Overdraw the colon with background to simulate blinking.
      int widthMin = tft.textWidth("00");
      int widthColon = tft.textWidth(":");
      int colonX = (tft.width() - kPad) - widthMin - widthColon;
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(COLOR_BG, COLOR_BG);
      tft.drawString(":", colonX, timeY);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(COLOR_TIME, COLOR_BG);
    }

    if (info.prevDay != lastPrevDay[i]) {
      int labelClearWidth = tft.width() / 2 - kPad;
      tft.fillRect(kPad, rowTop, labelClearWidth, rowHeight, COLOR_BG);
      tft.setTextFont(kFontLabel);
      tft.setTextColor(COLOR_LABEL, COLOR_BG);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(locations[i].label, kPad, rowTop + 2);
      if (info.prevDay) {
        tft.setTextFont(kFontNote);
        tft.setTextColor(TFT_YELLOW, COLOR_BG);
        tft.drawString("PREV DAY", kPad, rowTop + 2 + tft.fontHeight() + 2);
      }
    }

    tft.setTextFont(kFontTime);
    tft.setTextColor(COLOR_TIME, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    lastTimes[i] = info.timeStr;
    lastPrevDay[i] = info.prevDay;
    lastColonState[i] = info.showColon;
  }
}

// WiFiManager auto-connect (AP is shown on display if needed).
void connectWifi() {
  WiFi.mode(WIFI_STA);
  if (DEBUG_LOGS) {
    Serial.println("WiFi: starting config portal/auto-connect");
  }
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_LABEL, COLOR_BG);
  tft.setTextFont(kFontHeader);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WIFI SETUP", tft.width() / 2, tft.height() / 2 - 10);
  tft.drawString("AP: CYD-WORLD-CLOCK", tft.width() / 2,
                 tft.height() / 2 + 10);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);
  bool res = wm.autoConnect("CYD-World-Clock");
  if (DEBUG_LOGS) {
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
  }
  if (!res) {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(TFT_RED, COLOR_BG);
    tft.setTextFont(kFontHeader);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("WIFI FAILED", tft.width() / 2, tft.height() / 2);
    delay(3000);
  }
}

void setup() {
  // Hardware bring-up + time sync.
  Serial.begin(115200);
  delay(200);
  if (DEBUG_LOGS) {
    Serial.println("World clock boot");
  }

  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);

  if (DEBUG_LOGS) {
    Serial.println("TFT init...");
  }
  tft.init();
  if (DEBUG_LOGS) {
    Serial.println("TFT rotation...");
  }
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);

  connectWifi();
  if (!syncTime()) {
    if (DEBUG_LOGS) {
      Serial.println("NTP sync failed");
    }
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(TFT_RED, COLOR_BG);
    tft.setTextFont(kFontHeader);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("NTP FAILED", tft.width() / 2, tft.height() / 2);
    return;
  }

  if (DEBUG_LOGS) {
    Serial.println("Time synced");
  }
  drawStaticLayout();
}

void loop() {
  // Update header date and per-location times once per second.
  String dateStr = formatDate(SYDNEY_TZ);
  if (dateStr != lastDate) {
    drawHeaderDate(dateStr);
    lastDate = dateStr;
  }
  drawTimes();
  if (DEBUG_LOGS) {
    // Emit a readable timestamp once per second for debugging.
    unsigned long nowMs = millis();
    if (nowMs - lastDebugPrint >= 1000) {
      struct tm sydneyTm;
      getLocalTm(SYDNEY_TZ, time(nullptr), &sydneyTm);
      char buf[24];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &sydneyTm);
      Serial.print("Sydney time: ");
      Serial.println(buf);
      lastDebugPrint = nowMs;
    }
  }
  delay(1000);
}
