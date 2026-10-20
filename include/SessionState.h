#pragma once

// ============================================================
// SessionState.h
// Responsibility: Data structures and state management for the
// real-time video monitoring session.
// ============================================================

#include "Config.h"
#include <Arduino.h>

struct DetectionResult {
    bool     detected      = false;
    uint16_t maxSquareSize = 0;
    uint16_t squareX       = 0;
    uint16_t squareY       = 0;
    uint16_t frameWidth    = 0;
    uint16_t frameHeight   = 0;
};

struct MonitoringSession {
    bool     isRunning          = false;
    bool     stoppedByDetection = false;
    String   startTime          = "-";
    String   stopTime           = "-";
    String   stopReason         = "Aguardando inicio";
    uint32_t framesProcessed    = 0;
    uint16_t lastMaxSquareSize  = 0;
    uint16_t detectedSquareSize = 0;
    uint16_t detectedX          = 0;
    uint16_t detectedY          = 0;
    uint16_t minSquareSize      = DEFAULT_MIN_SQUARE_SIZE;
    uint8_t  blackThreshold     = DEFAULT_BLACK_THRESHOLD;
};

namespace SessionState {
    void                     init();
    void                     start();
    void                     stopManual();
    void                     stopOnDetection(const DetectionResult &result);
    void                     recordFrameResult(const DetectionResult &result);
    void                     setConfig(uint16_t minSquareSize, uint8_t blackThreshold);
    const MonitoringSession &get();
} // namespace SessionState
