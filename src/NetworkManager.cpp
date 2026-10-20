// ============================================================
// NetworkManager.cpp
// Responsibility: Initializes the ESP32 Wi-Fi network so that
// a smartphone can connect directly via Access Point or LAN.
// ============================================================

#include "NetworkManager.h"
#include "Config.h"
#include "Logger.h"
#include <WiFi.h>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef AP_SSID
#define AP_SSID "ESP32-Video-Monitor"
#endif

#ifndef AP_PASS
#define AP_PASS "12345678"
#endif

namespace NetworkManager {

    void begin() {
        String staSsid = String(WIFI_SSID);
        String staPass = String(WIFI_PASS);
        String apSsid  = String(AP_SSID);
        String apPass  = String(AP_PASS);

        if (apSsid.length() == 0) {
            apSsid = "ESP32-Video-Monitor";
        }

        WiFi.disconnect(true);
        delay(100);

        if (staSsid.length() > 0) {
            WiFi.mode(WIFI_AP_STA);
        } else {
            WiFi.mode(WIFI_AP);
        }

        bool apStarted = false;
        if (apPass.length() >= 8) {
            apStarted = WiFi.softAP(apSsid.c_str(), apPass.c_str());
        } else {
            apStarted = WiFi.softAP(apSsid.c_str());
        }

        if (apStarted) {
            Serial.printf("AP Started: %s | IP: %s\n", apSsid.c_str(),
                          WiFi.softAPIP().toString().c_str());
        } else {
            Serial.println("Failed to start SoftAP!");
        }

        if (staSsid.length() > 0) {
            Serial.printf("Connecting to Wi-Fi STA: %s", staSsid.c_str());
            WiFi.begin(staSsid.c_str(), staPass.c_str());
            for (uint8_t i = 0;
                 i < WIFI_CONNECT_TIMEOUT_HALF_S && WiFi.status() != WL_CONNECTED; i++) {
                delay(500);
                Serial.print(".");
            }
            Serial.println();
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("STA Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("STA connection timed out. Continuing in AP mode.");
            }
        }
    }

    String getAccessPointIp() {
        return WiFi.softAPIP().toString();
    }

    String getStationIp() {
        if (WiFi.status() == WL_CONNECTED) {
            return WiFi.localIP().toString();
        }
        return "";
    }

    String getAccessPointSsid() {
        String ssid = String(AP_SSID);
        return ssid.length() > 0 ? ssid : "ESP32-Video-Monitor";
    }

} // namespace NetworkManager
