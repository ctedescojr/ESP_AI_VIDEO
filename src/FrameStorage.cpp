// ============================================================
// FrameStorage.cpp
// Responsibility: Double-buffering of the last valid frame in
// RAM and SPIFFS persistence on stop events.
// ============================================================

#include "FrameStorage.h"
#include "Config.h"
#include "Logger.h"
#include <SPIFFS.h>
#include <vector>

static std::vector<uint8_t> lastValidFrameBuffer;
static bool                 savedOnDisk = false;

namespace FrameStorage {

    void init() {
        lastValidFrameBuffer.reserve(20480);
        savedOnDisk = SPIFFS.exists(PATH_LAST_VALID_JPG);
    }

    void clearMemoryBuffer() {
        lastValidFrameBuffer.clear();
    }

    bool updateValidFrame(const uint8_t *jpgData, size_t jpgLen) {
        if (jpgData == nullptr || jpgLen == 0 || jpgLen > MAX_JPEG_FRAME_BYTES) {
            return false;
        }
        lastValidFrameBuffer.assign(jpgData, jpgData + jpgLen);
        return true;
    }

    bool commitLastValidFrameToDisk() {
        if (lastValidFrameBuffer.empty()) {
            Logger::println("No previous valid frame in RAM to commit to SPIFFS.");
            return false;
        }

        File file = SPIFFS.open(PATH_LAST_VALID_JPG, FILE_WRITE);
        if (!file) {
            Logger::println("Failed to open SPIFFS file for writing last valid frame.");
            return false;
        }

        size_t written = file.write(lastValidFrameBuffer.data(), lastValidFrameBuffer.size());
        file.close();

        savedOnDisk = (written == lastValidFrameBuffer.size());
        Logger::printf("Saved last valid frame to SPIFFS (%u bytes)\n",
                       static_cast<unsigned int>(written));
        return savedOnDisk;
    }

    bool hasSavedFrame() {
        return !lastValidFrameBuffer.empty() || savedOnDisk || SPIFFS.exists(PATH_LAST_VALID_JPG);
    }

    const uint8_t *getMemoryFrameData() {
        return lastValidFrameBuffer.empty() ? nullptr : lastValidFrameBuffer.data();
    }

    size_t getMemoryFrameSize() {
        return lastValidFrameBuffer.size();
    }

} // namespace FrameStorage
