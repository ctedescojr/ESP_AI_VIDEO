#pragma once

// ============================================================
// VideoAnalyzer.h
// Responsibility: Decodes incoming JPEG frames on the ESP32
// and detects contiguous all-black square regions.
// ============================================================

#include "SessionState.h"
#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace VideoAnalyzer {
    DetectionResult analyzeJpegFrame(const uint8_t *jpgData, size_t jpgLen,
                                     uint16_t minSquareSize, uint8_t blackThreshold);
} // namespace VideoAnalyzer
