#include <Arduino.h>
#include "DLITracker.h"

DLITracker::DLITracker() {
    reset();
}

void DLITracker::reset() {
    // Reset DLI at the start of the day
    currentDLI = 0.0;
}

void DLITracker::update(float currentPPFD) {
    // Update DLI based on the current PPFD reading
    currentDLI += (currentPPFD * SAMPLE_PERIOD) / 1000000.0f; // Convert PPFD to DLI (mol/m²/day)
}

double DLITracker::getCurrentDLI() {
    return currentDLI;
}