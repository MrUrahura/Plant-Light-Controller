#include <Arduino.h>
#include "DLITracker.h"

DLITracker::DLITracker(const LightSensor& sensor)
    : sensor(sensor)
{
    reset();
}

void DLITracker::reset() {
    // Reset DLI at the start of the day
    currentDLI = 0.0;
}

void DLITracker::update() {
    // Update DLI based on the current PPFD reading
    currentDLI += (sensor.readPPFDLevel() * SAMPLE_PERIOD) / 1000000.0f; // Convert PPFD to DLI (mol/m²/day)
}

double DLITracker::getCurrentDLI() const {
    return currentDLI;
}