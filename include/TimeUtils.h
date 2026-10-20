#pragma once

// ============================================================
// TimeUtils.h
// Responsibility: Local date and time utilities (NTP & RTC).
// ============================================================

#include <Arduino.h>
#include <time.h>

namespace TimeUtils {
    void   initNtp();
    void   syncFromBrowser(time_t unixTimestamp);
    bool   isTimeSynchronized();
    String currentTimeFormatted();
    String currentDateFormatted();
    String currentDateTimeFormatted();
} // namespace TimeUtils
