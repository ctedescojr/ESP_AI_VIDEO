// ============================================================
// Logger.cpp
// Responsibility: Routing log messages to Serial and Telnet.
// ============================================================

#include "Logger.h"
#include "Config.h"
#include <cstdarg>

static WiFiServer telnetServer(TELNET_PORT);
static WiFiClient telnetClient;

static void sendToTelnet(const String &msg) {
    if (!telnetClient || !telnetClient.connected()) {
        return;
    }
    String normalized = msg;
    normalized.replace("\n", "\r\n");
    telnetClient.print(normalized);
}

static void acceptNewTelnetClient() {
    if (!telnetServer.hasClient()) {
        return;
    }

    if (!telnetClient || !telnetClient.connected()) {
        if (telnetClient) {
            telnetClient.stop();
        }
        telnetClient = telnetServer.available();
        telnetClient.println("--- ESP32 VIDEO MONITOR CONNECTED ---");
        telnetClient.flush();
    } else {
        telnetServer.available().stop();
    }
}

namespace Logger {

    void begin() {
        telnetServer.begin();
        telnetServer.setNoDelay(true);
        Serial.println("Telnet server started on port 23");
    }

    void handleTelnetClient() {
        acceptNewTelnetClient();
    }

    void print(const String &msg) {
        Serial.print(msg);
        sendToTelnet(msg);
    }

    void println(const String &msg) {
        Serial.println(msg);
        sendToTelnet(msg + "\n");
    }

    void printf(const char *format, ...) {
        char    buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        String msg(buffer);
        Serial.print(msg);
        sendToTelnet(msg);
    }

} // namespace Logger
