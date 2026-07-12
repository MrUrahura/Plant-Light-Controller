#include <Arduino.h>
#include "DLITracker.h"

DLITracker::DLITracker(const LightSensor& sensor, const TimeManager& timeManager)
    : sensor(sensor), timeManager(timeManager)
{

}

void DLITracker::loadDLI() {
    prefs.begin("DLITracker", true);
    currentDLI = prefs.getDouble("currentDLI", 0.0);
    lastCheckpointTime = static_cast<time_t>(prefs.getULong64("lastCheckpointTime", 0));
    prefs.end();

    lastUpdateTime = timeManager.getUnixTime();
}

void DLITracker::reset() {
    currentDLI = 0.0;

    time_t now = timeManager.getUnixTime();

    lastUpdateTime = now;
    lastCheckpointTime = now;

    saveDLI();
}

void DLITracker::update() {
    time_t now = timeManager.getUnixTime();

    double elapsedSeconds = difftime(now, lastUpdateTime);

    if (elapsedSeconds <= 0) return;

    // Convert PPFD to DLI (mol/m²/day)
    currentDLI += sensor.readPPFDLevel() * elapsedSeconds / 1000000.0;
    
    lastUpdateTime = now;

    // Save every 30 minutes
    if (difftime(now, lastCheckpointTime) >= 1800)
    {
        saveDLI();
        lastCheckpointTime = now;
    }
}

void DLITracker::saveDLI() {
    prefs.begin("DLITracker", false);
    currentDLI = prefs.putDouble("currentDLI", currentDLI);
    prefs.putULong64("lastCheckpointTime", static_cast<uint64_t>(lastCheckpointTime));
    prefs.end();
}

double DLITracker::getCurrentDLI() const {
    return currentDLI;
}