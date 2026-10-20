// ============================================================
// VideoAnalyzer.cpp
// Responsibility: Hardware-assisted JPEG decoding via
// esp_jpg_decode and O(W*H) dynamic programming maximal black
// square detection.
// ============================================================

#include "VideoAnalyzer.h"
#include "Config.h"
#include "Logger.h"
#include "esp_jpg_decode.h"
#include <algorithm>
#include <cstring>

// Static 1-bit-per-pixel mask for 320x240 frame (9,600 bytes)
static constexpr size_t BITMASK_BYTES = (FRAME_WIDTH * FRAME_HEIGHT) / 8;
static uint8_t          blackPixelMask[BITMASK_BYTES];
static uint16_t         dpRowPrev[FRAME_WIDTH];
static uint16_t         dpRowCurr[FRAME_WIDTH];

struct JpegDecodeContext {
    const uint8_t *jpgData        = nullptr;
    size_t         jpgLen         = 0;
    uint16_t       width          = 0;
    uint16_t       height         = 0;
    uint8_t        blackThreshold = DEFAULT_BLACK_THRESHOLD;
};

static inline void setBlackBit(uint16_t x, uint16_t y) {
    size_t bitIndex               = static_cast<size_t>(y) * FRAME_WIDTH + x;
    blackPixelMask[bitIndex >> 3] |= static_cast<uint8_t>(1U << (bitIndex & 7U));
}

static inline bool isBlackBit(uint16_t x, uint16_t y) {
    size_t bitIndex = static_cast<size_t>(y) * FRAME_WIDTH + x;
    return (blackPixelMask[bitIndex >> 3] & static_cast<uint8_t>(1U << (bitIndex & 7U))) != 0;
}

static size_t jpgReaderCallback(void *arg, size_t index, uint8_t *buf, size_t len) {
    JpegDecodeContext *ctx = static_cast<JpegDecodeContext *>(arg);
    if (index >= ctx->jpgLen) {
        return 0;
    }
    size_t available = ctx->jpgLen - index;
    size_t toRead    = (len < available) ? len : available;
    if (buf != nullptr && toRead > 0) {
        memcpy(buf, ctx->jpgData + index, toRead);
    }
    return toRead;
}

static bool jpgWriterCallback(void *arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                              uint8_t *data) {
    JpegDecodeContext *ctx = static_cast<JpegDecodeContext *>(arg);

    // Header notification from esp_jpg_decode
    if (data == nullptr) {
        ctx->width  = w;
        ctx->height = h;
        return true;
    }

    const uint8_t threshold = ctx->blackThreshold;

    for (uint16_t row = 0; row < h; row++) {
        uint16_t py = y + row;
        if (py >= FRAME_HEIGHT) {
            break;
        }
        const uint8_t *rowPtr = data + (static_cast<size_t>(row) * w * 3U);
        for (uint16_t col = 0; col < w; col++) {
            uint16_t px = x + col;
            if (px >= FRAME_WIDTH) {
                break;
            }
            const uint8_t *pixel = rowPtr + (static_cast<size_t>(col) * 3U);
            uint8_t        r     = pixel[0];
            uint8_t        g     = pixel[1];
            uint8_t        b     = pixel[2];

            // Fast integer luminance approximation: Y = (77*R + 150*G + 29*B) >> 8
            uint8_t luma = static_cast<uint8_t>((77U * r + 150U * g + 29U * b) >> 8);
            if (luma <= threshold) {
                setBlackBit(px, py);
            }
        }
    }

    return true;
}

namespace VideoAnalyzer {

    DetectionResult analyzeJpegFrame(const uint8_t *jpgData, size_t jpgLen,
                                     uint16_t minSquareSize, uint8_t blackThreshold) {
        DetectionResult result;
        if (jpgData == nullptr || jpgLen < 4) {
            return result;
        }

        memset(blackPixelMask, 0, sizeof(blackPixelMask));

        JpegDecodeContext ctx;
        ctx.jpgData        = jpgData;
        ctx.jpgLen         = jpgLen;
        ctx.blackThreshold = blackThreshold;

        esp_err_t err =
            esp_jpg_decode(jpgLen, JPG_SCALE_NONE, jpgReaderCallback, jpgWriterCallback, &ctx);
        if (err != ESP_OK || ctx.width == 0 || ctx.height == 0) {
            Logger::println("JPEG decode failed in VideoAnalyzer");
            return result;
        }

        uint16_t evalWidth  = std::min<uint16_t>(ctx.width, FRAME_WIDTH);
        uint16_t evalHeight = std::min<uint16_t>(ctx.height, FRAME_HEIGHT);

        result.frameWidth  = evalWidth;
        result.frameHeight = evalHeight;

        memset(dpRowPrev, 0, sizeof(dpRowPrev));
        memset(dpRowCurr, 0, sizeof(dpRowCurr));

        uint16_t bestSize         = 0;
        uint16_t bestBottomRightX = 0;
        uint16_t bestBottomRightY = 0;

        for (uint16_t y = 0; y < evalHeight; y++) {
            for (uint16_t x = 0; x < evalWidth; x++) {
                if (isBlackBit(x, y)) {
                    if (x == 0 || y == 0) {
                        dpRowCurr[x] = 1;
                    } else {
                        uint16_t up   = dpRowPrev[x];
                        uint16_t left = dpRowCurr[x - 1];
                        uint16_t diag = dpRowPrev[x - 1];
                        dpRowCurr[x]  = std::min({up, left, diag}) + 1;
                    }

                    if (dpRowCurr[x] > bestSize) {
                        bestSize         = dpRowCurr[x];
                        bestBottomRightX = x;
                        bestBottomRightY = y;
                    }
                } else {
                    dpRowCurr[x] = 0;
                }
            }
            memcpy(dpRowPrev, dpRowCurr, evalWidth * sizeof(uint16_t));
        }

        result.maxSquareSize = bestSize;
        if (bestSize > 0) {
            result.squareX = bestBottomRightX + 1 - bestSize;
            result.squareY = bestBottomRightY + 1 - bestSize;
        }
        result.detected = (bestSize >= minSquareSize);

        return result;
    }

} // namespace VideoAnalyzer
