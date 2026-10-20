// ============================================================
// script.js
// Responsibility: Rear camera capture, real-time JPEG frame
// transmission to ESP32, clock synchronization, and UI state.
// ============================================================

let mediaStream = null;
let useRearCamera = true;
let isMonitoring = false;
let isSendingFrame = false;
let frameLoopTimer = null;
let lastDetectedBox = null;

const FRAME_WIDTH = 320;
const FRAME_HEIGHT = 240;
const FRAME_INTERVAL_MS = 250;

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

async function initCamera() {
    const warningBox = document.getElementById("camera-permission-warning");
    const originSpan = document.getElementById("current-origin-url");
    if (originSpan) {
        originSpan.textContent = window.location.origin;
    }

    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
        if (warningBox) {
            warningBox.classList.remove("hidden");
        }
        startRenderLoop();
        return;
    }

    if (mediaStream) {
        mediaStream.getTracks().forEach(track => track.stop());
    }

    const constraints = {
        audio: false,
        video: {
            facingMode: useRearCamera ? { ideal: "environment" } : "user",
            width: { ideal: FRAME_WIDTH },
            height: { ideal: FRAME_HEIGHT }
        }
    };

    try {
        mediaStream = await navigator.mediaDevices.getUserMedia(constraints);
        const video = document.getElementById("camera-video");
        video.srcObject = mediaStream;
        await video.play();
        if (warningBox) {
            warningBox.classList.add("hidden");
        }
    } catch (err) {
        console.error("Error accessing camera:", err);
        if (warningBox) {
            warningBox.classList.remove("hidden");
        }
    }

    startRenderLoop();
}

function switchCamera() {
    useRearCamera = !useRearCamera;
    initCamera();
}

function drawCurrentFrameToCanvas() {
    const video = document.getElementById("camera-video");
    const canvas = document.getElementById("capture-canvas");
    const ctx = canvas.getContext("2d");

    if (video && video.readyState >= 2 && video.videoWidth > 0) {
        ctx.drawImage(video, 0, 0, FRAME_WIDTH, FRAME_HEIGHT);
    } else {
        // Fallback synthetic background when camera is unavailable
        const grad = ctx.createLinearGradient(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
        grad.addColorStop(0, "#1e3a8a");
        grad.addColorStop(1, "#3b82f6");
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

        ctx.fillStyle = "#ffffff";
        ctx.font = "14px sans-serif";
        ctx.fillText("Camera Live Preview (" + new Date().toLocaleTimeString() + ")", 24, 34);
    }

    // Optional test blackout square injection (80x80 solid black square)
    const simCheck = document.getElementById("chk-simulate-blackout");
    if (simCheck && simCheck.checked) {
        ctx.fillStyle = "#000000";
        ctx.fillRect(110, 70, 80, 80);
    }
}

function drawOverlayBox() {
    if (!lastDetectedBox) {
        return;
    }
    const canvas = document.getElementById("capture-canvas");
    const ctx = canvas.getContext("2d");
    ctx.strokeStyle = "#ef4444";
    ctx.lineWidth = 3;
    ctx.strokeRect(lastDetectedBox.x, lastDetectedBox.y, lastDetectedBox.size, lastDetectedBox.size);
}

function startRenderLoop() {
    function render() {
        drawCurrentFrameToCanvas();
        drawOverlayBox();
        requestAnimationFrame(render);
    }
    requestAnimationFrame(render);
}

async function startMonitoring() {
    lastDetectedBox = null;
    const simCheck = document.getElementById("chk-simulate-blackout");
    if (simCheck) {
        simCheck.checked = false;
    }

    try {
        const body = "timestamp=" + encodeURIComponent(getUnixTimestampSeconds());
        const res = await fetch("/start", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
        const data = await res.json();
        updateUiFromStatus(data);

        if (frameLoopTimer) {
            clearInterval(frameLoopTimer);
        }
        frameLoopTimer = setInterval(captureAndSendFrame, FRAME_INTERVAL_MS);
    } catch (err) {
        console.error("Failed to start monitoring:", err);
    }
}

async function stopMonitoring() {
    if (frameLoopTimer) {
        clearInterval(frameLoopTimer);
        frameLoopTimer = null;
    }
    try {
        const body = "timestamp=" + encodeURIComponent(getUnixTimestampSeconds());
        const res = await fetch("/stop", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: body
        });
        const data = await res.json();
        updateUiFromStatus(data);
        await loadStopHistory();
    } catch (err) {
        console.error("Failed to stop monitoring:", err);
    }
}

function captureAndSendFrame() {
    if (!isMonitoring || isSendingFrame) {
        return;
    }

    const canvas = document.getElementById("capture-canvas");
    drawCurrentFrameToCanvas();

    isSendingFrame = true;
    canvas.toBlob(async (blob) => {
        if (!blob) {
            isSendingFrame = false;
            return;
        }
        try {
            const formData = new FormData();
            formData.append("frame", blob, "frame.jpg");

            const res = await fetch("/frame", {
                method: "POST",
                body: formData
            });
            if (res.ok) {
                const status = await res.json();
                updateUiFromStatus(status);
            }
        } catch (err) {
            console.error("Error sending frame:", err);
        } finally {
            isSendingFrame = false;
        }
    }, "image/jpeg", 0.75);
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
            alert("Configuracao atualizada no ESP32!");
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

function updateUiFromStatus(data) {
    const wasMonitoring = isMonitoring;
    isMonitoring = Boolean(data.is_running);

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
    }

    const stopBanner = document.getElementById("auto-stop-banner");
    const stopBannerText = document.getElementById("stop-banner-text");
    if (data.stopped_by_detection) {
        if (frameLoopTimer) {
            clearInterval(frameLoopTimer);
            frameLoopTimer = null;
        }
        if (stopBanner) stopBanner.classList.remove("hidden");
        if (stopBannerText) {
            stopBannerText.textContent =
                "Quadrado preto de " + data.detected_square + "x" + data.detected_square +
                " px detectado em (" + data.detected_x + ", " + data.detected_y +
                ") às " + data.stop_time + ". Última imagem pré-falha salva!";
        }
        if (data.has_saved_image) {
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
    if (isMonitoring) {
        return;
    }
    try {
        const res = await fetch("/status");
        if (res.ok) {
            const data = await res.json();
            updateUiFromStatus(data);
            if (data.has_saved_image) {
                refreshSavedImage();
            }
        }
    } catch (err) {
        console.warn("Status poll error:", err);
    }
}

window.addEventListener("DOMContentLoaded", async () => {
    await syncBrowserTimeWithEsp();
    await initCamera();
    await pollStatus();
    await loadStopHistory();
    setInterval(pollStatus, 5000);
});
