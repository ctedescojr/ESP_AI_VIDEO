#pragma once

// ============================================================
// Logger.h
// Responsibility: Dual logging output via Serial and Telnet.
// ============================================================

#include <Arduino.h>
#include <WiFi.h>

namespace Logger {
    void begin();
    void handleTelnetClient();

    void print(const String &msg);
    void println(const String &msg = "");
    void printf(const char *format, ...);
} // namespace Logger
