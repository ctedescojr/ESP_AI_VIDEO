// ============================================================
// HistoryStorage.cpp
// Responsibility: Reading and writing the stop history CSV file
// in SPIFFS and managing the in-memory history list.
// ============================================================

#include "HistoryStorage.h"
#include "Config.h"
#include "Logger.h"
#include <SPIFFS.h>

static std::vector<StopHistoryEntry> entries;
static uint32_t                      nextEntryId = 1;

static String sanitizeCsvField(const String &input) {
    String out = input;
    out.replace(";", ",");
    out.replace("\n", " ");
    out.replace("\r", "");
    return out;
}

static bool parseCsvLine(const String &line, StopHistoryEntry &outEntry) {
    String fields[9];
    int    fieldIdx = 0;
    int    startPos = 0;

    for (int i = 0; i < static_cast<int>(line.length()) && fieldIdx < 9; i++) {
        if (line[i] == ';') {
            fields[fieldIdx++] = line.substring(startPos, i);
            startPos           = i + 1;
        }
    }
    if (fieldIdx < 9) {
        fields[fieldIdx++] = line.substring(startPos);
    }
    if (fieldIdx < 9) {
        return false;
    }

    outEntry.id             = static_cast<uint32_t>(fields[0].toInt());
    outEntry.startTime      = fields[1];
    outEntry.stopTime       = fields[2];
    outEntry.stopType       = fields[3];
    outEntry.reason         = fields[4];
    outEntry.squareSize     = static_cast<uint16_t>(fields[5].toInt());
    outEntry.coordX         = static_cast<uint16_t>(fields[6].toInt());
    outEntry.coordY         = static_cast<uint16_t>(fields[7].toInt());
    outEntry.framesAnalyzed = static_cast<uint32_t>(fields[8].toInt());
    return outEntry.id > 0;
}

static void loadHistoryFromSpiffs() {
    entries.clear();
    nextEntryId = 1;

    if (!SPIFFS.exists(PATH_CSV_HISTORY)) {
        File f = SPIFFS.open(PATH_CSV_HISTORY, FILE_WRITE);
        if (f) {
            f.print(CSV_HISTORY_HEADER);
            f.close();
        }
        return;
    }

    File f = SPIFFS.open(PATH_CSV_HISTORY, FILE_READ);
    if (!f) {
        return;
    }

    bool isFirstLine = true;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) {
            continue;
        }
        if (isFirstLine && line.startsWith("ID;")) {
            isFirstLine = false;
            continue;
        }
        isFirstLine = false;

        StopHistoryEntry item;
        if (parseCsvLine(line, item)) {
            if (entries.size() >= MAX_HISTORY_RAM_ITEMS) {
                entries.erase(entries.begin());
            }
            entries.push_back(item);
            if (item.id >= nextEntryId) {
                nextEntryId = item.id + 1;
            }
        }
    }
    f.close();
    Logger::printf("HistoryStorage loaded %u stop records from SPIFFS.\n",
                   static_cast<unsigned int>(entries.size()));
}

namespace HistoryStorage {

    void init() {
        entries.reserve(MAX_HISTORY_RAM_ITEMS);
        loadHistoryFromSpiffs();
    }

    void appendStopEvent(const MonitoringSession &session) {
        StopHistoryEntry item;
        item.id             = nextEntryId++;
        item.startTime      = sanitizeCsvField(session.startTime);
        item.stopTime       = sanitizeCsvField(session.stopTime);
        item.stopType       = session.stoppedByDetection ? "AUTOMATICA" : "MANUAL";
        item.reason         = sanitizeCsvField(session.stopReason);
        item.squareSize     = session.stoppedByDetection ? session.detectedSquareSize
                                                         : session.lastMaxSquareSize;
        item.coordX         = session.detectedX;
        item.coordY         = session.detectedY;
        item.framesAnalyzed = session.framesProcessed;

        if (entries.size() >= MAX_HISTORY_RAM_ITEMS) {
            entries.erase(entries.begin());
        }
        entries.push_back(item);

        if (!SPIFFS.exists(PATH_CSV_HISTORY)) {
            File headerFile = SPIFFS.open(PATH_CSV_HISTORY, FILE_WRITE);
            if (headerFile) {
                headerFile.print(CSV_HISTORY_HEADER);
                headerFile.close();
            }
        }

        File f = SPIFFS.open(PATH_CSV_HISTORY, FILE_APPEND);
        if (!f) {
            Logger::println("Failed to open stop_history.csv for appending.");
            return;
        }

        String csvLine = String(item.id) + ";" + item.startTime + ";" + item.stopTime + ";" +
                         item.stopType + ";" + item.reason + ";" + String(item.squareSize) + ";" +
                         String(item.coordX) + ";" + String(item.coordY) + ";" +
                         String(item.framesAnalyzed) + "\n";
        f.print(csvLine);
        f.flush();
        f.close();

        Logger::printf("Stop event #%u saved to history CSV.\n", item.id);
    }

    void clearHistory() {
        entries.clear();
        nextEntryId = 1;

        File f = SPIFFS.open(PATH_CSV_HISTORY, FILE_WRITE);
        if (f) {
            f.print(CSV_HISTORY_HEADER);
            f.close();
        }
        Logger::println("Stop history cleared.");
    }

    const std::vector<StopHistoryEntry> &getEntries() {
        return entries;
    }

} // namespace HistoryStorage
