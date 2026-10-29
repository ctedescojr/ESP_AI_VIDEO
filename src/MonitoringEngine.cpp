// ============================================================
// MonitoringEngine.cpp
// Responsibility: Coordinates periodic OV2640 frame capture
// and dispatches frames to VideoAnalyzer and FrameStorage
// whenever a monitoring session is active.
// ============================================================

#include "MonitoringEngine.h"
#include "CameraManager.h"
#include "Config.h"
#include "FrameStorage.h"
#include "SessionState.h"
#include "VideoAnalyzer.h"
#include <Arduino.h>

static unsigned long lastCaptureMs = 0;

static void analyzeLatestFrameIfRunning() {
    const MonitoringSession &session = SessionState::get();
    if (!session.isRunning) {
        return;
    }

    const uint8_t *frameData = CameraManager::getLatestFrameData();
    size_t         frameSize = CameraManager::getLatestFrameSize();
    if (frameData == nullptr || frameSize == 0) {
        return;
    }

    DetectionResult result = VideoAnalyzer::analyzeJpegFrame(
        frameData, frameSize, session.minSquareSize, session.blackThreshold);

    SessionState::recordFrameResult(result);

    if (result.detected) {
        if (FrameStorage::getMemoryFrameSize() == 0) {
            FrameStorage::updateValidFrame(frameData, frameSize);
        }
        FrameStorage::commitLastValidFrameToDisk();
        SessionState::stopOnDetection(result);
    } else {
        FrameStorage::updateValidFrame(frameData, frameSize);
    }
}

namespace MonitoringEngine {

    void loop() {
        if (!CameraManager::isReady()) {
            return;
        }

        unsigned long now = millis();
        if (now - lastCaptureMs < CAMERA_CAPTURE_INTERVAL_MS) {
            return;
        }
        lastCaptureMs = now;

        if (CameraManager::captureFrame()) {
            analyzeLatestFrameIfRunning();
        }
    }

} // namespace MonitoringEngine
