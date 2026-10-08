#pragma once

#include <Arduino.h>

// Leveled debug macros for modules outside main.cpp (src/network/improv_setup.cpp).
//
// main.cpp keeps its own DBG_* set, where every caller passes its own "\n".
// These print the same way (level tag, timestamp, diagnostics log buffer) but
// append the newline, as the cyd-web-installer copy-in expects. main.cpp must
// not include this header, or the two sets collide.

// Runtime level, set via /api/debug-level. Defined in main.cpp.
extern uint8_t debugLevel;
String getDebugTimestamp();
void addToLogBuffer(uint8_t level, const char *msg);

// clang-format off
#define DBG_LOG_(lvl, tag, fmt, ...) do { if (debugLevel >= (lvl)) { \
    Serial.printf(tag " %s " fmt "\n", getDebugTimestamp().c_str(), ##__VA_ARGS__); \
    char buf_[80]; snprintf(buf_, sizeof(buf_), fmt, ##__VA_ARGS__); \
    addToLogBuffer((lvl), buf_); } } while (0)
#define DBG_ERROR(fmt, ...)   DBG_LOG_(1, "[ERR ]", fmt, ##__VA_ARGS__)
#define DBG_WARN(fmt, ...)    DBG_LOG_(2, "[WARN]", fmt, ##__VA_ARGS__)
#define DBG_INFO(fmt, ...)    DBG_LOG_(3, "[INFO]", fmt, ##__VA_ARGS__)
#define DBG_VERBOSE(fmt, ...) DBG_LOG_(4, "[VERB]", fmt, ##__VA_ARGS__)
// clang-format on
