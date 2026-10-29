// ============================================================
// script.js
// Responsibility: Polls live JPEG snapshots from ESP32-CAM
// OV2640 sensor, synchronizes clock, and updates session UI.
// ============================================================

let isMonitoring = false;
let isFetchingSnapshot = false;
let flashLedEnabled = false;
let cameraFlipped180 = false;
let lastDetectedBox = null;

const CANVAS_WIDTH = 320;
const CANVAS_HEIGHT = 240;
const SNAPSHOT_INTERVAL_MS = 280;
const STATUS_INTERVAL_MS = 600;

function getUnixTimestampSeconds() {
    return Math.floor(Date.now() / 1000);
}

async function syncBrowserTimeWithEsp() {
    try {
        const body = "timestamp=" + encodeURIComponent(getUnixTimestampSeconds());
        await fetch("/sync_time", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
    } catch (err) {
        console.warn("Time sync error:", err);
    }
}

function drawOverlayBox(ctx) {
    if (!lastDetectedBox) {
        return;
    }
    ctx.strokeStyle = "#ef4444";
    ctx.lineWidth = 3;
    ctx.strokeRect(
        lastDetectedBox.x,
        lastDetectedBox.y,
        lastDetectedBox.size,
        lastDetectedBox.size
    );
}

function drawPlaceholderOnCanvas(message) {
    const canvas = document.getElementById("capture-canvas");
    if (!canvas) {
        return;
    }
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#0f172a";
    ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
    ctx.fillStyle = "#94a3b8";
    ctx.font = "14px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(message, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2);
}

function fetchLiveSnapshot() {
    if (isFetchingSnapshot || document.hidden) {
        return;
    }
    isFetchingSnapshot = true;

    const img = new Image();
    img.onload = () => {
        const canvas = document.getElementById("capture-canvas");
        if (canvas) {
            const ctx = canvas.getContext("2d");
            ctx.drawImage(img, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
            drawOverlayBox(ctx);
        }
        isFetchingSnapshot = false;
    };
    img.onerror = () => {
        drawPlaceholderOnCanvas("Aguardando frame da ESP32-CAM...");
        isFetchingSnapshot = false;
    };
    img.src = "/snapshot?t=" + Date.now();
}

async function toggleFlashLed() {
    const targetState = !flashLedEnabled;
    try {
        const res = await fetch("/camera_control", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: "flash=" + (targetState ? "1" : "0")
        });
        if (res.ok) {
            const status = await res.json();
            updateUiFromStatus(status);
        }
    } catch (err) {
        console.error("Failed to toggle flash LED:", err);
    }
}

async function toggleCameraFlip() {
    const targetFlip = !cameraFlipped180;
    try {
        const res = await fetch("/camera_control", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: "flip=" + (targetFlip ? "1" : "0")
        });
        if (res.ok) {
            const status = await res.json();
            updateUiFromStatus(status);
        }
    } catch (err) {
        console.error("Failed to toggle camera flip:", err);
    }
}

async function startMonitoring() {
    lastDetectedBox = null;
    try {
        const body = "timestamp=" + encodeURIComponent(getUnixTimestampSeconds());
        const res = await fetch("/start", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
        if (res.ok) {
            const data = await res.json();
            updateUiFromStatus(data);
        }
    } catch (err) {
        console.error("Failed to start monitoring:", err);
    }
}

async function stopMonitoring() {
    try {
        const body = "timestamp=" + encodeURIComponent(getUnixTimestampSeconds());
        const res = await fetch("/stop", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
        if (res.ok) {
            const data = await res.json();
            updateUiFromStatus(data);
            await loadStopHistory();
        }
    } catch (err) {
        console.error("Failed to stop monitoring:", err);
    }
}

async function saveDetectionConfig() {
    const minSquare = document.getElementById("inp-min-square").value;
    const threshold = document.getElementById("inp-threshold").value;

    try {
        const body = "min_square=" + encodeURIComponent(minSquare) +
                     "&threshold=" + encodeURIComponent(threshold);
        const res = await fetch("/config", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
        if (res.ok) {
            const status = await res.json();
            updateUiFromStatus(status);
            alert("Configuracao atualizada na ESP32-CAM!");
        }
    } catch (err) {
        alert("Erro ao salvar configuracao.");
    }
}

function refreshSavedImage() {
    const img = document.getElementById("saved-last-img");
    const placeholder = document.getElementById("no-image-placeholder");
    const downloadBtn = document.getElementById("btn-download-img");

    const url = "/last_image?t=" + Date.now();
    img.onload = () => {
        img.classList.remove("hidden");
        if (placeholder) placeholder.classList.add("hidden");
        if (downloadBtn) {
            downloadBtn.href = url;
            downloadBtn.classList.remove("hidden");
        }
    };
    img.src = url;
}

async function loadStopHistory() {
    try {
        const res = await fetch("/history");
        if (!res.ok) {
            return;
        }
        const list = await res.json();
        const tbody = document.getElementById("history-tbody");
        if (!tbody) {
            return;
        }
        if (!list || list.length === 0) {
            tbody.innerHTML =
                '<tr><td colspan="8" class="empty-table-msg">Nenhuma parada registrada.</td></tr>';
            return;
        }
        tbody.innerHTML = list.map(item => {
            const isAuto = item.stop_type === "AUTOMATICA";
            const badgeClass = isAuto ? "type-badge type-auto" : "type-badge type-manual";
            const badgeLabel = isAuto ? "Automática" : "Manual";
            const coords = isAuto ? `X=${item.coord_x}, Y=${item.coord_y}` : "-";
            return `<tr>
                <td><strong>#${item.id}</strong></td>
                <td>${item.start_time}</td>
                <td>${item.stop_time}</td>
                <td><span class="${badgeClass}">${badgeLabel}</span></td>
                <td>${item.reason}</td>
                <td>${item.square_size}x${item.square_size} px</td>
                <td>${coords}</td>
                <td>${item.frames}</td>
            </tr>`;
        }).join("");
    } catch (err) {
        console.warn("Failed to load history:", err);
    }
}

async function clearStopHistory() {
    if (!confirm("Deseja realmente apagar todo o histórico de paradas?")) {
        return;
    }
    try {
        const res = await fetch("/clear_history", { method: "POST" });
        if (res.ok) {
            await loadStopHistory();
        }
    } catch (err) {
        alert("Erro ao limpar histórico.");
    }
}

function updateHardwareBadges(data) {
    const camBadge = document.getElementById("cam-badge");
    const camWarning = document.getElementById("camera-error-warning");
    if (camBadge) {
        camBadge.className = "badge " + (data.camera_ok ? "badge-running" : "badge-alert");
        camBadge.textContent = "OV2640: " + (data.camera_ok ? "OK" : "ERRO");
    }
    if (camWarning) {
        if (data.camera_ok === false) {
            camWarning.classList.remove("hidden");
        } else {
            camWarning.classList.add("hidden");
        }
    }

    const psramBadge = document.getElementById("psram-badge");
    if (psramBadge) {
        if (data.psram_ok) {
            const psramKb = Math.round((data.free_psram || 0) / 1024);
            psramBadge.className = "badge badge-running";
            psramBadge.textContent = "PSRAM: " + psramKb + " KB";
        } else {
            psramBadge.className = "badge badge-idle";
            psramBadge.textContent = "PSRAM: N/A";
        }
    }

    flashLedEnabled = Boolean(data.flash_on);
    cameraFlipped180 = Boolean(data.flipped_180);

    const btnFlash = document.getElementById("btn-flash");
    if (btnFlash) {
        btnFlash.textContent = flashLedEnabled ? "💡 Flash: ON" : "💡 Flash: OFF";
        btnFlash.classList.toggle("btn-flash-active", flashLedEnabled);
    }
}

function updateUiFromStatus(data) {
    const wasMonitoring = isMonitoring;
    isMonitoring = Boolean(data.is_running);

    updateHardwareBadges(data);

    const btnStart = document.getElementById("btn-start");
    const btnStop = document.getElementById("btn-stop");
    if (btnStart) btnStart.disabled = isMonitoring;
    if (btnStop) btnStop.disabled = !isMonitoring;

    const statusBadge = document.getElementById("status-badge");
    if (statusBadge) {
        statusBadge.className = "badge";
        if (data.is_running) {
            statusBadge.classList.add("badge-running");
            statusBadge.textContent = "Monitorando";
        } else if (data.stopped_by_detection) {
            statusBadge.classList.add("badge-alert");
            statusBadge.textContent = "Parada Automatica";
        } else {
            statusBadge.classList.add("badge-idle");
            statusBadge.textContent = "Parado";
        }
    }

    const heapBadge = document.getElementById("heap-badge");
    if (heapBadge && data.free_heap) {
        heapBadge.textContent = "RAM: " + Math.round(data.free_heap / 1024) + " KB";
    }

    document.getElementById("val-start-time").textContent = data.start_time || "-";
    document.getElementById("val-stop-time").textContent = data.stop_time || "-";
    document.getElementById("val-stop-reason").textContent = data.stop_reason || "-";

    const sq = data.stopped_by_detection ? data.detected_square : data.last_max_square;
    document.getElementById("val-max-square").textContent = (sq || 0) + "x" + (sq || 0) + " px";
    document.getElementById("val-frames").textContent = data.frames_processed || 0;

    if (data.stopped_by_detection && data.detected_square > 0) {
        document.getElementById("val-coords").textContent =
            "X=" + data.detected_x + ", Y=" + data.detected_y +
            " (" + data.detected_square + "x" + data.detected_square + ")";
        lastDetectedBox = {
            x: data.detected_x,
            y: data.detected_y,
            size: data.detected_square
        };
    } else if (data.is_running) {
        document.getElementById("val-coords").textContent = "-";
        lastDetectedBox = null;
    }

    const stopBanner = document.getElementById("auto-stop-banner");
    const stopBannerText = document.getElementById("stop-banner-text");
    if (data.stopped_by_detection) {
        if (stopBanner) stopBanner.classList.remove("hidden");
        if (stopBannerText) {
            stopBannerText.textContent =
                "Quadrado preto de " + data.detected_square + "x" + data.detected_square +
                " px detectado em (" + data.detected_x + ", " + data.detected_y +
                ") às " + data.stop_time + ". Última imagem pré-falha salva!";
        }
        if (wasMonitoring && data.has_saved_image) {
            refreshSavedImage();
        }
        if (wasMonitoring) {
            loadStopHistory();
        }
    } else {
        if (stopBanner) stopBanner.classList.add("hidden");
    }
}

async function pollStatus() {
    try {
        const res = await fetch("/status");
        if (res.ok) {
            const data = await res.json();
            updateUiFromStatus(data);
        }
    } catch (err) {
        console.warn("Status poll error:", err);
    }
}

window.addEventListener("DOMContentLoaded", async () => {
    drawPlaceholderOnCanvas("Conectando à câmera OV2640...");
    await syncBrowserTimeWithEsp();
    await pollStatus();
    await loadStopHistory();

    const initialStatusRes = await fetch("/status").catch(() => null);
    if (initialStatusRes && initialStatusRes.ok) {
        const initialData = await initialStatusRes.json();
        if (initialData.has_saved_image) {
            refreshSavedImage();
        }
    }

    setInterval(fetchLiveSnapshot, SNAPSHOT_INTERVAL_MS);
    setInterval(pollStatus, STATUS_INTERVAL_MS);
});
