#pragma once
#include "Plant.h"
#include "LightSensor.h"
#include "ServoController.h"
#include "WeatherForecast.h"
#include "TimeManager.h"
#include "DLITracker.h"

class LightController {
public:
    LightController(const Plant& plant, const LightSensor& sensor, ServoController& shades, const DLITracker& dliTracker, const WeatherForecast& weather, const TimeManager& timeManager);
    void optimizeState();

private:
    const Plant& plant;
    const LightSensor& sensor;
    ServoController& shades;
    const DLITracker& dliTracker;
    const WeatherForecast& weather;
    const TimeManager& timeManager;
    double currentPlannedDLI;

    void updatePlannedHourlyDLI();
    uint8_t findOptimumState();
};