#pragma once

// ============================================================
// FrameStorage.h
// Responsibility: Keeps the latest valid (pre-blackout) JPEG
// frame in RAM and persists it to SPIFFS when a blackout stop
// event occurs.
// ============================================================

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace FrameStorage {
    void           init();
    void           clearMemoryBuffer();
    bool           updateValidFrame(const uint8_t *jpgData, size_t jpgLen);
    bool           commitLastValidFrameToDisk();
    bool           hasSavedFrame();
    const uint8_t *getMemoryFrameData();
    size_t         getMemoryFrameSize();
} // namespace FrameStorage
