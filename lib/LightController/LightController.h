#pragma once
#include <Arduino.h>
#include <array>
#include "Plant.h"
#include "LightSensor.h"
#include "ServoController.h"
#include "DLITracker.h"
#include "WeatherForecast.h"
#include "TimeManager.h"

class LightController {
public:
    LightController(
        const Plant& plant,
        const LightSensor& sensor,
        ServoController& shades,
        const DLITracker& dliTracker,
        const WeatherForecast& weather,
        const TimeManager& timeManager
    );

    void optimizeState();
    void update();

    bool isCurrentlyOptimizing() const {
        return isOptimizing;
    }

private:
    const Plant& plant;
    const LightSensor& sensor;
    ServoController& shades;
    const DLITracker& dliTracker;
    const WeatherForecast& weather;
    const TimeManager& timeManager;

    double currentPlannedDLI = 0.0;
    void updatePlannedHourlyDLI();
    double calculateCostForState(uint8_t state, double measuredPPFD);

    bool isOptimizing = false;
    uint8_t currentTestingState = 0;
    bool waitingForMovement = false;
    bool applyingFinalState = false;
    uint32_t movementStartTime = 0;

    static constexpr uint32_t SETTLE_DELAY_MS = 200;

    std::array<double, 8> measuredPPFDs = {0.0};
};