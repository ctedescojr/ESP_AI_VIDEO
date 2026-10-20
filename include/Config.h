#pragma once

// ============================================================
// Config.h
// Responsibility: Global hardware constants, video analysis
// thresholds, network settings, and filesystem paths.
// ============================================================

#include <Arduino.h>

// ── Hardware Pins ───────────────────────────────────────────
static constexpr uint8_t PIN_LED_STATUS = 2;
static constexpr uint8_t PIN_LED_ALERT  = 4;

// ── Watchdog ────────────────────────────────────────────────
static constexpr uint32_t WDT_TIMEOUT_S = 15;

// ── Network & Server ────────────────────────────────────────
static constexpr uint16_t WEB_SERVER_PORT             = 80;
static constexpr uint16_t TELNET_PORT                 = 23;
static constexpr uint8_t  WIFI_CONNECT_TIMEOUT_HALF_S = 16;

// ── Video Analysis Settings ─────────────────────────────────
static constexpr uint16_t FRAME_WIDTH             = 320;
static constexpr uint16_t FRAME_HEIGHT            = 240;
static constexpr size_t   MAX_JPEG_FRAME_BYTES    = 45000;
static constexpr uint16_t DEFAULT_MIN_SQUARE_SIZE = 50;
static constexpr uint16_t MIN_ALLOWED_SQUARE_SIZE = 20;
static constexpr uint16_t MAX_ALLOWED_SQUARE_SIZE = 200;
static constexpr uint8_t  DEFAULT_BLACK_THRESHOLD = 35;

// ── SPIFFS File Paths ───────────────────────────────────────
static constexpr char PATH_HTML_INDEX[]     = "/index.html";
static constexpr char PATH_CSS_STYLE[]      = "/style.css";
static constexpr char PATH_JS_SCRIPT[]      = "/script.js";
static constexpr char PATH_LAST_VALID_JPG[] = "/last_valid.jpg";
static constexpr char PATH_CSV_HISTORY[]    = "/stop_history.csv";

// ── History Storage ─────────────────────────────────────────
static constexpr size_t MAX_HISTORY_RAM_ITEMS = 50;
static constexpr char   CSV_HISTORY_HEADER[]  =
    "ID;Hora_Inicio;Hora_Parada;Tipo_Parada;Motivo;Tamanho_Quadrado_Px;Coord_X;Coord_Y;Frames_Analisados\n";
