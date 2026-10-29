#pragma once

// ============================================================
// CameraManager.h
// Responsibility: Hardware abstraction for the ESP32-CAM
// OV2640 sensor, frame capture buffering, and camera controls.
// ============================================================

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace CameraManager {
    bool           init();
    bool           isReady();
    bool           captureFrame();
    bool           hasLatestFrame();
    const uint8_t *getLatestFrameData();
    size_t         getLatestFrameSize();

    void setFlashLed(bool enabled);
    bool isFlashLedEnabled();
    void setFlip180(bool enabled);
    bool isFlipped180();
} // namespace CameraManager
