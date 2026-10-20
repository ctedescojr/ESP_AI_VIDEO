#pragma once

// ============================================================
// HistoryStorage.h
// Responsibility: Persistent CSV storage in SPIFFS and in-memory
// cache for monitoring stop events.
// ============================================================

#include "SessionState.h"
#include <Arduino.h>
#include <vector>

struct StopHistoryEntry {
    uint32_t id             = 0;
    String   startTime      = "-";
    String   stopTime       = "-";
    String   stopType       = "-";
    String   reason         = "-";
    uint16_t squareSize     = 0;
    uint16_t coordX         = 0;
    uint16_t coordY         = 0;
    uint32_t framesAnalyzed = 0;
};

namespace HistoryStorage {
    void                                 init();
    void                                 appendStopEvent(const MonitoringSession &session);
    void                                 clearHistory();
    const std::vector<StopHistoryEntry> &getEntries();
} // namespace HistoryStorage
