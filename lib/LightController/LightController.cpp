#include "LightController.h"
#include <cmath>

LightController::LightController(const Plant& plant, const LightSensor& sensor, ServoController& shades, const DLITracker& dliTracker, const WeatherForecast& weather, const TimeManager& timeManager)
    : plant(plant), sensor(sensor), shades(shades), dliTracker(dliTracker), weather(weather), timeManager(timeManager) {}

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
}

void LightController::update() {
    if (!isOptimizing) return;

    // Phase 1: Issue the physical move command for the current state profile
    if (!waitingForMovement) {
        Serial.print("LightController: Initiating movement to test state [");
        Serial.print(currentTestingState);
        Serial.println("]");
        
        shades.setShades(currentTestingState);
        waitingForMovement = true;
        movementStartTime = millis(); 
        return; 
    }

    // Phase 2: Dynamic Hardware Verification Guard
    if (waitingForMovement) {
        // If the servos are actively spinning, yield execution immediately
        if (shades.isMoving()) {
            return; 
        }
        
        // Small stabilization cushion (e.g., 200ms) to make sure the light readings settle
        if (millis() - movementStartTime < SETTLE_DELAY_MS) {
            return; 
        }
        
        // Phase 3: Hardware is static and stable. Record data!
        double physicalReading = sensor.readPPFDLevel();
        measuredPPFDs[currentTestingState] = physicalReading;
        
        Serial.print("LightController: Captured Real PPFD for State ");
        Serial.print(currentTestingState);
        Serial.print(" -> ");
        Serial.println(physicalReading);

        // Advance slot pointer and clear wait lock
        currentTestingState++;
        waitingForMovement = false; 

        // Phase 4: All profiles checked. Run the cost equation matrix.
        if (currentTestingState > 7) {
            double minCost = 9999999.0; // Initialize to a very high cost
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
            
            shades.setShades(minState);
            isOptimizing = false; // Release global system lock
        }
    }
}

double LightController::calculateCostForState(uint8_t state, double measuredPPFD) {
    double cost = 0.0;

    int hour, minute, second;
    timeManager.getCurrentTime(hour, minute, second);
    int timeInHour = (minute * 60) + second;
    
    double planError = (currentPlannedDLI > 0.0) ? (dliTracker.getCurrentDLI() / currentPlannedDLI) - (timeInHour / 3600.0) : 0.0;
    if (std::abs(planError) < 0.03) planError = 0;
    double planWeight = 10.0 + 50.0 * std::abs(planError);

    double targetPPFD = plant.getTargetPPFD();
    double ppfdError = measuredPPFD - targetPPFD;

    if ((planError > 0 && ppfdError > 0) || (planError < 0 && ppfdError < 0)) {
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
    
    // Enforce valid 24-hour array tracking index range, so no overnight photoperiods
    if (startHour < 0 || startHour >= 24 || endHour > 24 || startHour >= endHour) {
        currentPlannedDLI = 0.0; // Clear state safely
        return;
    }

    std::array<double, 24> hourlyDLI = weather.getHourlyDLI();
    double physicalDLILeft = weather.getRemainingDLI();
    
    // Protect against missing data, division-by-zero, or infinitesimal floats
    if (physicalDLILeft <= 0.001) {
        Serial.println("LightController: Minimal or zero physical solar DLI remaining today.");
        currentPlannedDLI = 0.0; // Explicitly reset tracking metrics
        return;
    }

    double plantNeededDLI = std::max(0.0, plant.getTargetDLI() - dliTracker.getCurrentDLI());
    double targetDLIAllocation = std::min(physicalDLILeft, plantNeededDLI);
    
    currentPlannedDLI = (targetDLIAllocation * (hourlyDLI[startHour] / physicalDLILeft));
}