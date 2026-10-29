// ============================================================
// main.cpp
// Responsibility: Orchestration of setup() and loop().
//
// Zero business logic here — only module initialization in the
// proper order and event dispatching inside loop().
// ============================================================

#include <Arduino.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <esp_task_wdt.h>

#include "CameraManager.h"
#include "Config.h"
#include "FrameStorage.h"
#include "HistoryStorage.h"
#include "Logger.h"
#include "MonitoringEngine.h"
#include "NetworkManager.h"
#include "SessionState.h"
#include "TimeUtils.h"
#include "WebRoutes.h"

static WebServer server(WEB_SERVER_PORT);

static void initHardware() {
    pinMode(PIN_LED_ONBOARD_RED, OUTPUT);
    pinMode(PIN_LED_FLASH, OUTPUT);

    digitalWrite(PIN_LED_ONBOARD_RED, LED_ONBOARD_OFF);
    digitalWrite(PIN_LED_FLASH, LOW);
}

static bool initSpiffs() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }
    return true;
}

// ────────────────────────────────────────────────────────────
// SETUP
// ────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    initHardware();

    if (!initSpiffs()) {
        return;
    }

    CameraManager::init();
    FrameStorage::init();
    HistoryStorage::init();
    SessionState::init();

    NetworkManager::begin();
    Logger::begin();
    TimeUtils::initNtp();

    WebRoutes::registerAll(server);
    server.begin();

    Logger::println("HTTP WebServer started!");
    Logger::println("AP IP: " + NetworkManager::getAccessPointIp());

    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(nullptr);
    Logger::printf("Watchdog enabled (%us)\n", WDT_TIMEOUT_S);
}

// ────────────────────────────────────────────────────────────
// LOOP
// ────────────────────────────────────────────────────────────

void loop() {
    esp_task_wdt_reset();

    Logger::handleTelnetClient();
    server.handleClient();
    MonitoringEngine::loop();
}
