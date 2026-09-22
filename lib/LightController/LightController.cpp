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
    currentTestingState = 0;
    waitingForMovement = false;
    applyingFinalState = false;
}

void LightController::update() {
    if (!isOptimizing) {
        return;
    }

    // Final state has already been selected. Wait until the physical movement is actually complete before releasing the optimization lock.
    if (applyingFinalState) {

        if (shades.isMoving()) {
            return;
        }

        if (millis() - movementStartTime < SETTLE_DELAY_MS) {
            return;
        }

        applyingFinalState = false;
        waitingForMovement = false;
        isOptimizing = false;

        Serial.println("LightController: Final shade position stabilized.");
        return;
    }

    // Phase 1: command the next test state.
    if (!waitingForMovement) {
        Serial.print("LightController: Initiating movement to test state [");
        Serial.print(currentTestingState);
        Serial.println("]");

        shades.setShades(currentTestingState);

        waitingForMovement = true;
        movementStartTime = millis();
        return;
    }

    // Phase 2: wait for all commanded servos to physically stop.
    if (shades.isMoving()) {
        return;
    }

    // Phase 3: stabilization delay after movement.
    if (millis() - movementStartTime < SETTLE_DELAY_MS) {
        return;
    }

    // Phase 4: measure the current physical configuration.
    double physicalReading = sensor.readPPFDLevel();
    measuredPPFDs[currentTestingState] = physicalReading;

    Serial.print("LightController: Captured Real PPFD for State ");
    Serial.print(currentTestingState);
    Serial.print(" -> ");
    Serial.println(physicalReading);

    currentTestingState++;
    waitingForMovement = false;

    // All eight states have been measured.
    if (currentTestingState > 7) {

        double minCost = 9999999.0;
        uint8_t minState = 0;

        for (uint8_t i = 0; i <= 7; ++i) {
            double cost = calculateCostForState(i, measuredPPFDs[i]);

            if (i == 0 || cost < minCost) {
                minCost = cost;
                minState = i;
            }
        }

        Serial.print("\n>>> Optimization Complete! Optimal Configuration State Applied: ");
        Serial.println(minState);

        // Command the final physical position.
        shades.setShades(minState);

        // Do NOT mark optimization finished yet.
        // The main loop must wait for the final movement to finish.
        applyingFinalState = true;
        movementStartTime = millis();
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