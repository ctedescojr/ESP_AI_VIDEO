// ============================================================
// CameraManager.cpp
// Responsibility: Configures and operates the OV2640 camera on
// AI-Thinker ESP32-CAM with PSRAM buffering.
// ============================================================

#include "CameraManager.h"
#include "Config.h"
#include "Logger.h"
#include "esp_camera.h"
#include <vector>

static bool                 cameraReady  = false;
static bool                 flashEnabled = false;
static bool                 flipped180   = false;
static std::vector<uint8_t> latestJpegBuffer;

static camera_config_t buildCameraConfig() {
    camera_config_t config = {};
    config.ledc_channel    = LEDC_CHANNEL_0;
    config.ledc_timer      = LEDC_TIMER_0;
    config.pin_d0          = CAM_PIN_D0;
    config.pin_d1          = CAM_PIN_D1;
    config.pin_d2          = CAM_PIN_D2;
    config.pin_d3          = CAM_PIN_D3;
    config.pin_d4          = CAM_PIN_D4;
    config.pin_d5          = CAM_PIN_D5;
    config.pin_d6          = CAM_PIN_D6;
    config.pin_d7          = CAM_PIN_D7;
    config.pin_xclk        = CAM_PIN_XCLK;
    config.pin_pclk        = CAM_PIN_PCLK;
    config.pin_vsync       = CAM_PIN_VSYNC;
    config.pin_href        = CAM_PIN_HREF;
    config.pin_sccb_sda    = CAM_PIN_SIOD;
    config.pin_sccb_scl    = CAM_PIN_SIOC;
    config.pin_pwdn        = CAM_PIN_PWDN;
    config.pin_reset       = CAM_PIN_RESET;
    config.xclk_freq_hz    = CAMERA_XCLK_FREQ_HZ;
    config.pixel_format    = PIXFORMAT_JPEG;
    config.frame_size      = FRAMESIZE_QVGA;
    config.jpeg_quality    = CAMERA_JPEG_QUALITY;

    if (psramFound()) {
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.fb_count    = 2;
        config.grab_mode   = CAMERA_GRAB_LATEST;
    } else {
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count    = 1;
        config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
    }

    return config;
}

static void applyDefaultSensorSettings() {
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == nullptr) {
        return;
    }
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
    sensor->set_quality(sensor, CAMERA_JPEG_QUALITY);
    sensor->set_vflip(sensor, 0);
    sensor->set_hmirror(sensor, 0);
}

namespace CameraManager {

    bool init() {
        latestJpegBuffer.reserve(24576);

        camera_config_t config = buildCameraConfig();
        esp_err_t       err    = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x\n", err);
            cameraReady = false;
            return false;
        }

        applyDefaultSensorSettings();
        cameraReady = true;
        Serial.printf("OV2640 Camera initialized (PSRAM: %s)\n", psramFound() ? "YES" : "NO");
        return true;
    }

    bool isReady() {
        return cameraReady;
    }

    bool captureFrame() {
        if (!cameraReady) {
            return false;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            Logger::println("Camera frame capture failed.");
            return false;
        }

        bool copied = false;
        if (fb->len > 0 && fb->len <= MAX_JPEG_FRAME_BYTES) {
            latestJpegBuffer.assign(fb->buf, fb->buf + fb->len);
            copied = true;
        }

        esp_camera_fb_return(fb);
        return copied;
    }

    bool hasLatestFrame() {
        return !latestJpegBuffer.empty();
    }

    const uint8_t *getLatestFrameData() {
        return latestJpegBuffer.empty() ? nullptr : latestJpegBuffer.data();
    }

    size_t getLatestFrameSize() {
        return latestJpegBuffer.size();
    }

    void setFlashLed(bool enabled) {
        flashEnabled = enabled;
        digitalWrite(PIN_LED_FLASH, flashEnabled ? HIGH : LOW);
    }

    bool isFlashLedEnabled() {
        return flashEnabled;
    }

    void setFlip180(bool enabled) {
        flipped180       = enabled;
        sensor_t *sensor = esp_camera_sensor_get();
        if (sensor != nullptr) {
            sensor->set_vflip(sensor, flipped180 ? 1 : 0);
            sensor->set_hmirror(sensor, flipped180 ? 1 : 0);
        }
    }

    bool isFlipped180() {
        return flipped180;
    }

} // namespace CameraManager
