// ============================================================
// WebRoutes.cpp
// Responsibility: Implementation of all HTTP routes.
//
// Endpoints:
//   GET  /                — Serves index.html
//   GET  /style.css       — Serves style.css
//   GET  /script.js       — Serves script.js
//   GET  /snapshot        — Serves latest live JPEG from OV2640
//   POST /camera_control  — Controls Flash LED and 180 flip
//   POST /sync_time       — Synchronizes ESP32 RTC with browser
//   POST /start           — Starts monitoring session
//   POST /stop            — Stops monitoring session manually
//   POST /config          — Updates square size & luma threshold
//   GET  /status          — Returns JSON session & system status
//   GET  /last_image      — Serves last valid JPEG before blackout
//   GET  /history         — Returns JSON stop history list
//   GET  /download_history— Downloads stop_history.csv
//   POST /clear_history   — Clears stop history
// ============================================================

#include "WebRoutes.h"
#include "CameraManager.h"
#include "Config.h"
#include "FrameStorage.h"
#include "HistoryStorage.h"
#include "Logger.h"
#include "NetworkManager.h"
#include "SessionState.h"
#include "TimeUtils.h"
#include <SPIFFS.h>

static String jsonEscape(const String &s) {
    String out;
    out.reserve(s.length());
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else {
            out += c;
        }
    }
    return out;
}

static void serveSpiffsFile(WebServer &server, const char *path, const char *mime) {
    if (!SPIFFS.exists(path)) {
        server.send(404, "text/plain", String("File not found: ") + path);
        return;
    }
    File f = SPIFFS.open(path, "r");
    server.streamFile(f, mime);
    f.close();
}

static String buildStatusJson() {
    const MonitoringSession &s       = SessionState::get();
    size_t                   total   = SPIFFS.totalBytes();
    size_t                   used    = SPIFFS.usedBytes();
    float                    diskPct = (total > 0) ? (used * 100.0f / total) : 0.0f;

    String json = "{";
    json += "\"is_running\":" + String(s.isRunning ? "true" : "false") + ",";
    json += "\"stopped_by_detection\":" + String(s.stoppedByDetection ? "true" : "false") + ",";
    json += "\"start_time\":\"" + jsonEscape(s.startTime) + "\",";
    json += "\"stop_time\":\"" + jsonEscape(s.stopTime) + "\",";
    json += "\"stop_reason\":\"" + jsonEscape(s.stopReason) + "\",";
    json += "\"frames_processed\":" + String(s.framesProcessed) + ",";
    json += "\"last_max_square\":" + String(s.lastMaxSquareSize) + ",";
    json += "\"detected_square\":" + String(s.detectedSquareSize) + ",";
    json += "\"detected_x\":" + String(s.detectedX) + ",";
    json += "\"detected_y\":" + String(s.detectedY) + ",";
    json += "\"min_square_size\":" + String(s.minSquareSize) + ",";
    json += "\"black_threshold\":" + String(s.blackThreshold) + ",";
    json += "\"has_saved_image\":" + String(FrameStorage::hasSavedFrame() ? "true" : "false") + ",";
    json += "\"camera_ok\":" + String(CameraManager::isReady() ? "true" : "false") + ",";
    json += "\"psram_ok\":" + String(psramFound() ? "true" : "false") + ",";
    json += "\"free_psram\":" + String(ESP.getFreePsram()) + ",";
    json += "\"flash_on\":" + String(CameraManager::isFlashLedEnabled() ? "true" : "false") + ",";
    json += "\"flipped_180\":" + String(CameraManager::isFlipped180() ? "true" : "false") + ",";
    json += "\"ap_ip\":\"" + NetworkManager::getAccessPointIp() + "\",";
    json += "\"sta_ip\":\"" + NetworkManager::getStationIp() + "\",";
    json += "\"disk_pct\":" + String(diskPct, 1) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "}";
    return json;
}

static void handleSnapshot(WebServer &server) {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    if (!CameraManager::isReady()) {
        server.send(503, "text/plain", "Camera not initialized");
        return;
    }
    if (!CameraManager::hasLatestFrame()) {
        CameraManager::captureFrame();
    }
    const uint8_t *data = CameraManager::getLatestFrameData();
    size_t         size = CameraManager::getLatestFrameSize();
    if (data == nullptr || size == 0) {
        server.send(503, "text/plain", "No camera frame available");
        return;
    }
    server.send_P(200, "image/jpeg", reinterpret_cast<const char *>(data), size);
}

static void handleCameraControl(WebServer &server) {
    if (server.hasArg("flash")) {
        bool enableFlash = (server.arg("flash") == "1" || server.arg("flash") == "true");
        CameraManager::setFlashLed(enableFlash);
    }
    if (server.hasArg("flip")) {
        bool enableFlip = (server.arg("flip") == "1" || server.arg("flip") == "true");
        CameraManager::setFlip180(enableFlip);
    }
    server.send(200, "application/json", buildStatusJson());
}

static void handleStart(WebServer &server) {
    if (server.hasArg("timestamp")) {
        time_t ts = static_cast<time_t>(server.arg("timestamp").toInt());
        TimeUtils::syncFromBrowser(ts);
    }
    FrameStorage::clearMemoryBuffer();
    SessionState::start();
    server.send(200, "application/json", buildStatusJson());
}

static void handleStop(WebServer &server) {
    if (server.hasArg("timestamp")) {
        time_t ts = static_cast<time_t>(server.arg("timestamp").toInt());
        TimeUtils::syncFromBrowser(ts);
    }
    SessionState::stopManual();
    server.send(200, "application/json", buildStatusJson());
}

static void handleSyncTime(WebServer &server) {
    if (!server.hasArg("timestamp")) {
        server.send(400, "text/plain", "Missing timestamp");
        return;
    }
    time_t ts = static_cast<time_t>(server.arg("timestamp").toInt());
    TimeUtils::syncFromBrowser(ts);
    server.send(200, "text/plain", "OK");
}

static void handleConfig(WebServer &server) {
    const MonitoringSession &current   = SessionState::get();
    uint16_t                 minSquare = current.minSquareSize;
    uint8_t                  threshold = current.blackThreshold;

    if (server.hasArg("min_square")) {
        minSquare = static_cast<uint16_t>(server.arg("min_square").toInt());
    }
    if (server.hasArg("threshold")) {
        int val   = server.arg("threshold").toInt();
        threshold = static_cast<uint8_t>(constrain(val, 1, 250));
    }

    SessionState::setConfig(minSquare, threshold);
    server.send(200, "application/json", buildStatusJson());
}

static void handleLastImage(WebServer &server) {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    if (FrameStorage::getMemoryFrameSize() > 0 && FrameStorage::getMemoryFrameData() != nullptr) {
        server.send_P(200, "image/jpeg",
                      reinterpret_cast<const char *>(FrameStorage::getMemoryFrameData()),
                      FrameStorage::getMemoryFrameSize());
        return;
    }
    if (SPIFFS.exists(PATH_LAST_VALID_JPG)) {
        serveSpiffsFile(server, PATH_LAST_VALID_JPG, "image/jpeg");
        return;
    }
    server.send(404, "text/plain", "No saved frame available");
}

static void handleHistoryJson(WebServer &server) {
    const auto &list = HistoryStorage::getEntries();
    String      json = "[";
    for (int i = static_cast<int>(list.size()) - 1; i >= 0; i--) {
        const auto &item = list[i];
        if (i < static_cast<int>(list.size()) - 1) {
            json += ",";
        }
        json += "{";
        json += "\"id\":" + String(item.id) + ",";
        json += "\"start_time\":\"" + jsonEscape(item.startTime) + "\",";
        json += "\"stop_time\":\"" + jsonEscape(item.stopTime) + "\",";
        json += "\"stop_type\":\"" + jsonEscape(item.stopType) + "\",";
        json += "\"reason\":\"" + jsonEscape(item.reason) + "\",";
        json += "\"square_size\":" + String(item.squareSize) + ",";
        json += "\"coord_x\":" + String(item.coordX) + ",";
        json += "\"coord_y\":" + String(item.coordY) + ",";
        json += "\"frames\":" + String(item.framesAnalyzed);
        json += "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

static void handleDownloadHistory(WebServer &server) {
    if (!SPIFFS.exists(PATH_CSV_HISTORY)) {
        server.send(404, "text/plain", "History file not found");
        return;
    }
    server.sendHeader("Content-Disposition", "attachment; filename=\"historico_paradas.csv\"");
    serveSpiffsFile(server, PATH_CSV_HISTORY, "text/csv");
}

static void handleClearHistory(WebServer &server) {
    HistoryStorage::clearHistory();
    server.send(200, "application/json", "{\"status\":\"cleared\"}");
}

namespace WebRoutes {

    void registerAll(WebServer &server) {
        server.on("/", HTTP_GET, [&]() { serveSpiffsFile(server, PATH_HTML_INDEX, "text/html"); });
        server.on("/style.css", HTTP_GET,
                  [&]() { serveSpiffsFile(server, PATH_CSS_STYLE, "text/css"); });
        server.on("/script.js", HTTP_GET,
                  [&]() { serveSpiffsFile(server, PATH_JS_SCRIPT, "application/javascript"); });

        server.on("/snapshot", HTTP_GET, [&]() { handleSnapshot(server); });
        server.on("/camera_control", HTTP_POST, [&]() { handleCameraControl(server); });
        server.on("/status", HTTP_GET,
                  [&]() { server.send(200, "application/json", buildStatusJson()); });
        server.on("/sync_time", HTTP_POST, [&]() { handleSyncTime(server); });
        server.on("/start", HTTP_POST, [&]() { handleStart(server); });
        server.on("/stop", HTTP_POST, [&]() { handleStop(server); });
        server.on("/config", HTTP_POST, [&]() { handleConfig(server); });
        server.on("/last_image", HTTP_GET, [&]() { handleLastImage(server); });
        server.on("/history", HTTP_GET, [&]() { handleHistoryJson(server); });
        server.on("/download_history", HTTP_GET, [&]() { handleDownloadHistory(server); });
        server.on("/clear_history", HTTP_POST, [&]() { handleClearHistory(server); });
    }

} // namespace WebRoutes
