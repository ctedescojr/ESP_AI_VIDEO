// ============================================================
// TimeUtils.cpp
// Responsibility: Local time abstraction with NTP and browser
// timestamp synchronization.
// ============================================================

#include "TimeUtils.h"
#include "Logger.h"
#include <sys/time.h>

static const char *BRAZIL_TZ = "BRT3BRST,M10.3.0/0,M2.3.0/0";

static bool getLocalTimeinfo(struct tm &out) {
    time_t now = time(nullptr);
    if (now < 1000000000L) {
        return false;
    }
    return localtime_r(&now, &out) != nullptr;
}

namespace TimeUtils {

    void initNtp() {
        setenv("TZ", BRAZIL_TZ, 1);
        tzset();
        configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");
    }

    void syncFromBrowser(time_t unixTimestamp) {
        if (unixTimestamp < 1000000000L) {
            return;
        }
        setenv("TZ", BRAZIL_TZ, 1);
        tzset();
        struct timeval tv = {unixTimestamp, 0};
        settimeofday(&tv, nullptr);
        Logger::printf("Time synchronized from browser (timestamp: %lu)\n",
                       static_cast<unsigned long>(unixTimestamp));
    }

    bool isTimeSynchronized() {
        return time(nullptr) > 1000000000L;
    }

    String currentTimeFormatted() {
        struct tm t;
        if (!getLocalTimeinfo(t)) {
            unsigned long totalSec = millis() / 1000UL;
            unsigned long hrs      = (totalSec / 3600UL) % 24UL;
            unsigned long mins     = (totalSec / 60UL) % 60UL;
            unsigned long secs     = totalSec % 60UL;
            char          fallback[16];
            snprintf(fallback, sizeof(fallback), "%02lu:%02lu:%02lu", hrs, mins, secs);
            return String(fallback);
        }
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
        return String(buffer);
    }

    String currentDateFormatted() {
        struct tm t;
        if (!getLocalTimeinfo(t)) {
            return "00/00/0000";
        }
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y", &t);
        return String(buffer);
    }

    String currentDateTimeFormatted() {
        if (!isTimeSynchronized()) {
            return currentTimeFormatted();
        }
        return currentDateFormatted() + " " + currentTimeFormatted();
    }

} // namespace TimeUtils
