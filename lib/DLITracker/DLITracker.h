#pragma once
#include "LightSensor.h"

class DLITracker {
public:
    DLITracker(const LightSensor& sensor);    
    void reset();
    void update();
    double getCurrentDLI() const;

private:
    double currentDLI;
    const LightSensor& sensor;
    const float SAMPLE_PERIOD = 60.0f; // in seconds
};