#include "LightController.h"
#include <cmath>

LightController::LightController(
    const Plant& plant,
    const LightSensor& sensor,
    ServoController& shades,
    const DLITracker& dliTracker,
    const WeatherForecast& weather,
    const TimeManager& timeManager
)
    : plant(plant),
    sensor(sensor),
    shades(shades),
    dliTracker(dliTracker),
    weather(weather),
    timeManager(timeManager)
{
}

void LightController::optimizeState() {
    if (isOptimizing) {
        Serial.println("LightController: Scan sequence already running.");
        return;
    }

    Serial.println("LightController: Starting physical shade sweep...");

    updatePlannedHourlyDLI();

    isOptimizing = true;
    currentIndex = 0;
    scanPhase = ScanPhase::COMMAND;
}

void LightController::update() {

    if (!isOptimizing) {
        return;
    }

    switch (scanPhase) {
        case ScanPhase::COMMAND:
        {
            uint8_t targetState = SCAN_ORDER[currentIndex];
            Serial.print("LightController: Initiating movement to test state [");
            Serial.print(targetState);
            Serial.println("]");

            // Only advance if ServoController actually accepted it.
            if (shades.setShades(targetState, false)) {
                scanPhase = ScanPhase::WAIT_FOR_MOVEMENT;
            }

            return;
        }

        case ScanPhase::WAIT_FOR_MOVEMENT:
        {
            // ServoController is the authority on physical movement.
            if (shades.isMoving()) {
                return;
            }

            // Movement finished. Begin stabilization NOW.
            settleStartTime = millis();
            scanPhase = ScanPhase::SETTLE;

            return;
        }

        case ScanPhase::SETTLE:
        {
            if (millis() - settleStartTime < SETTLE_DELAY_MS) {
                return;
            }

            scanPhase = ScanPhase::MEASURE;

            return;
        }

        case ScanPhase::MEASURE:
        {
            uint8_t measuredState = SCAN_ORDER[currentIndex];
            double physicalReading = sensor.readPPFDLevel();
            measuredPPFDs[measuredState] = physicalReading;

            Serial.print("LightController: Captured Real PPFD for State ");
            Serial.print(currentIndex);
            Serial.print(" -> ");
            Serial.println(physicalReading);

            currentIndex++;

            if (currentIndex <= 7) {
                scanPhase = ScanPhase::COMMAND;
                return;
            }

            // All eight states measured.
            double minCost = 9999999.0;
            finalState = 0;

            for (uint8_t i = 0; i <= 7; ++i) {

                double cost =
                    calculateCostForState(i, measuredPPFDs[i]);

                if (i == 0 || cost < minCost) {
                    minCost = cost;
                    finalState = i;
                }
            }

            Serial.print("\n>>> Optimization Complete! Optimal Configuration State Applied: ");
            Serial.println(finalState);

            scanPhase = ScanPhase::FINAL_COMMAND;

            return;
        }

        case ScanPhase::FINAL_COMMAND:
        {
            if (shades.setShades(finalState, true)) {
                scanPhase = ScanPhase::FINAL_WAIT;
            }

            return;
        }

        case ScanPhase::FINAL_WAIT:
        {
            if (shades.isMoving()) {
                return;
            }

            settleStartTime = millis();
            scanPhase = ScanPhase::FINAL_SETTLE;

            return;
        }

        case ScanPhase::FINAL_SETTLE:
        {
            if (millis() - settleStartTime < SETTLE_DELAY_MS) {
                return;
            }

            isOptimizing = false;
            scanPhase = ScanPhase::IDLE;

            Serial.println(
                "LightController: Final shade position stabilized."
            );

            return;
        }

        case ScanPhase::IDLE:
        {
            return;
        }
    }
}

double LightController::calculateCostForState(
    uint8_t state,
    double measuredPPFD
) {
    double cost = 0.0;

    int hour, minute, second;
    timeManager.getCurrentTime(hour, minute, second);
    int timeInHour = (minute * 60) + second;

    double planError = (currentPlannedDLI > 0.0) ? (dliTracker.getCurrentDLI() / currentPlannedDLI) - (timeInHour / 3600.0) : 0.0;

    if (std::abs(planError) < 0.03) {
        planError = 0;
    }

    double planWeight = 10.0 + 50.0 * std::abs(planError);

    double targetPPFD = plant.getTargetPPFD();
    double ppfdError = measuredPPFD - targetPPFD;

    if ((planError > 0 && ppfdError > 0) ||
        (planError < 0 && ppfdError < 0)) {
        cost += planWeight * ppfdError * ppfdError;
    }

    cost += ppfdError * ppfdError;

    if ((targetPPFD < 20.0)
     || (targetPPFD < 50.0 && measuredPPFD > 1.2 * targetPPFD)
     || (targetPPFD < 200.0 && measuredPPFD > 1.5 * targetPPFD)
     || (targetPPFD < 600.0 && measuredPPFD > 2.0 * targetPPFD)
     || (measuredPPFD > 2.5 * targetPPFD)) {
        cost += 10000.0;
    }

    return cost;
}

void LightController::updatePlannedHourlyDLI() {
    int startHour = timeManager.getHour();
    int endHour = timeManager.getPhotoperiodEndTime();

    if (startHour < 0 || startHour >= 24 || endHour > 24 || startHour >= endHour) {
        currentPlannedDLI = 0.0;
        return;
    }

    std::array<double, 24> hourlyDLI = weather.getHourlyDLI();
    double physicalDLILeft = weather.getRemainingDLI();

    if (physicalDLILeft <= 0.001) {
        Serial.println("LightController: Minimal or zero physical solar DLI remaining today.");
        currentPlannedDLI = 0.0;
        return;
    }

    double plantNeededDLI = std::max(0.0, plant.getTargetDLI() - dliTracker.getCurrentDLI());

    double targetDLIAllocation = std::min(physicalDLILeft, plantNeededDLI);

    currentPlannedDLI = targetDLIAllocation * (hourlyDLI[startHour] / physicalDLILeft);
}