#pragma once
#include <Preferences.h>
#include "LightSensor.h"
#include "TimeManager.h"

class DLITracker {
public:
    DLITracker(const LightSensor& sensor, const TimeManager& timeManager);
    void loadDLI();
    void reset();
    void update();
    double getCurrentDLI() const;

private:
    void saveDLI();

    double currentDLI;
    const LightSensor& sensor;
    const TimeManager& timeManager;
    time_t lastUpdateTime;
    time_t lastCheckpointTime;
    Preferences prefs;
};