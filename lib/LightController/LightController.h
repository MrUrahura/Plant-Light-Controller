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

    bool optimizeState();
    void cancelOptimization();
    void update();
    bool optimizationSucceeded() const {
        return optimizationCompleted;
    }

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
    bool optimizationCompleted = false;
    uint8_t currentIndex = 0;
    enum class ScanPhase {
        IDLE,
        COMMAND,
        WAIT_FOR_MOVEMENT,
        SETTLE,
        MEASURE,
        FINAL_COMMAND,
        FINAL_WAIT,
        FINAL_SETTLE
    };
    ScanPhase scanPhase = ScanPhase::IDLE;

    const uint8_t SCAN_ORDER[8] = {7, 6, 4, 5, 1, 0, 2, 3};

    uint32_t settleStartTime = 0;
    uint32_t lastSensorErrorLog = 0;
    uint32_t lastSensorReadAttempt = 0;
    uint32_t sensorFailureStartTime = 0;
    bool sensorFailureTiming = false;
    uint8_t finalState = 0;
    static constexpr uint32_t SETTLE_DELAY_MS = 200;
    static constexpr uint32_t SENSOR_RETRY_INTERVAL_MS = 1000;
    static constexpr uint32_t SENSOR_FAILURE_TIMEOUT_MS = 30000;

    std::array<double, 8> measuredPPFDs = {0.0};
};