#pragma once

// ============================================================
// WebRoutes.h
// Responsibility: Registers all HTTP endpoints for the video
// monitoring dashboard and frame ingestion API.
// ============================================================

#include <WebServer.h>

namespace WebRoutes {
    void registerAll(WebServer &server);
} // namespace WebRoutes
