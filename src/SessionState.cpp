// ============================================================
// SessionState.cpp
// Responsibility: State transitions for video monitoring,
// start/stop timestamps, and detection metadata.
// ============================================================

#include "SessionState.h"
#include "HistoryStorage.h"
#include "Logger.h"
#include "TimeUtils.h"

static MonitoringSession session;

namespace SessionState {

    void init() {
        session                = MonitoringSession();
        session.minSquareSize  = DEFAULT_MIN_SQUARE_SIZE;
        session.blackThreshold = DEFAULT_BLACK_THRESHOLD;
        session.stopReason     = "Aguardando inicio";
    }

    void start() {
        session.isRunning          = true;
        session.stoppedByDetection = false;
        session.startTime          = TimeUtils::currentDateTimeFormatted();
        session.stopTime           = "-";
        session.stopReason         = "Monitorando em tempo real";
        session.framesProcessed    = 0;
        session.lastMaxSquareSize  = 0;
        session.detectedSquareSize = 0;
        session.detectedX          = 0;
        session.detectedY          = 0;

        digitalWrite(PIN_LED_ONBOARD_RED, LED_ONBOARD_ON);

        Logger::printf("Session started at %s (minSquare=%u, threshold=%u)\n",
                       session.startTime.c_str(), session.minSquareSize, session.blackThreshold);
    }

    void stopManual() {
        if (!session.isRunning) {
            return;
        }
        session.isRunning          = false;
        session.stoppedByDetection = false;
        session.stopTime           = TimeUtils::currentDateTimeFormatted();
        session.stopReason         = "Parada manual pelo operador";

        digitalWrite(PIN_LED_ONBOARD_RED, LED_ONBOARD_OFF);
        HistoryStorage::appendStopEvent(session);
        Logger::printf("Session manually stopped at %s\n", session.stopTime.c_str());
    }

    void stopOnDetection(const DetectionResult &result) {
        session.isRunning          = false;
        session.stoppedByDetection = true;
        session.stopTime           = TimeUtils::currentDateTimeFormatted();
        session.lastMaxSquareSize  = result.maxSquareSize;
        session.detectedSquareSize = result.maxSquareSize;
        session.detectedX          = result.squareX;
        session.detectedY          = result.squareY;
        session.stopReason         = "Regiao preta detectada (" +
                             String(result.maxSquareSize) + "x" +
                             String(result.maxSquareSize) + " px)";

        digitalWrite(PIN_LED_ONBOARD_RED, LED_ONBOARD_OFF);
        HistoryStorage::appendStopEvent(session);

        Logger::printf("AUTO-STOP at %s! Black square %ux%u detected at (%u, %u)\n",
                       session.stopTime.c_str(), result.maxSquareSize, result.maxSquareSize,
                       result.squareX, result.squareY);
    }

    void recordFrameResult(const DetectionResult &result) {
        session.framesProcessed++;
        session.lastMaxSquareSize = result.maxSquareSize;
    }

    void setConfig(uint16_t minSquareSize, uint8_t blackThreshold) {
        if (minSquareSize < MIN_ALLOWED_SQUARE_SIZE) {
            minSquareSize = MIN_ALLOWED_SQUARE_SIZE;
        }
        if (minSquareSize > MAX_ALLOWED_SQUARE_SIZE) {
            minSquareSize = MAX_ALLOWED_SQUARE_SIZE;
        }
        session.minSquareSize  = minSquareSize;
        session.blackThreshold = blackThreshold;
        Logger::printf("Detection config updated: minSquare=%u px, threshold=%u\n",
                       session.minSquareSize, session.blackThreshold);
    }

    const MonitoringSession &get() {
        return session;
    }

} // namespace SessionState
