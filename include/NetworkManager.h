#pragma once

// ============================================================
// NetworkManager.h
// Responsibility: Wi-Fi Access Point and optional Station
// connection management.
// ============================================================

#include <Arduino.h>

namespace NetworkManager {
    void   begin();
    String getAccessPointIp();
    String getStationIp();
    String getAccessPointSsid();
} // namespace NetworkManager
