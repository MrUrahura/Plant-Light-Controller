#include <Arduino.h>
#include "DLITracker.h"

DLITracker::DLITracker(const LightSensor& sensor, const TimeManager& timeManager)
    : sensor(sensor), timeManager(timeManager)
{

}

void DLITracker::loadDLI() {
    prefs.begin("DLITracker", true);
    currentDLI = prefs.getDouble("currentDLI", 0.0);
    lastCheckpointTime = static_cast<time_t>(prefs.getULong64("lastChkTime", 0));
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
    if(!timeManager.withinPhotoperiod()){
        if(currentDLI > 0.0) {
            reset();
        } else {
            lastUpdateTime = timeManager.getUnixTime();
        }
        return;
    }
    
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
    Serial.println("Saving DLI info...");
    prefs.begin("DLITracker", false);
    prefs.putDouble("currentDLI", currentDLI);
    prefs.putULong64("lastChkTime", static_cast<uint64_t>(lastCheckpointTime));
    prefs.end();
}

double DLITracker::getCurrentDLI() const {
    return currentDLI;
}