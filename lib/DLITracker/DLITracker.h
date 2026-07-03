#pragma once

class DLITracker {
public:
    DLITracker();    
    void reset();
    void update();
    double getCurrentDLI();

private:
    double currentDLI;
    const float SAMPLE_PERIOD = 60.0f; // seconds
};