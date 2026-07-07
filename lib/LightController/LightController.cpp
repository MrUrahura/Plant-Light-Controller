#include <Arduino.h>
#include <cmath>
#include "LightController.h"

LightController::LightController(const Plant& plant, const LightSensor& sensor, ServoController& shades, const DLITracker& dliTracker, const WeatherForecast& weather, const TimeManager& timeManager)
    : plant(plant), sensor(sensor), shades(shades), dliTracker(dliTracker), weather(weather), timeManager(timeManager)
{

}

void LightController::optimizeState() {
    shades.setShades(findOptimumState());
}

void LightController::updatePlannedHourlyDLI() {
    int startHour = timeManager.getHour();
    int endHour = timeManager.getPhotoperiodEndTime();
    if (startHour < 0 || endHour > 24 || startHour >= endHour) return;

    std::array<double, 24> hourlyDLI = weather.getHourlyDLI();
    double physicalDLILeft = weather.getRemainingDLI();
    if (physicalDLILeft <= 0.0) {
        Serial.println("LightController: No physical solar DLI remaining for the day.");
        return;
    }

    double plantNeededDLI = max(0.0, plant.getTargetDLI() - dliTracker.getCurrentDLI());
    double targetDLIAllocation = min(physicalDLILeft, plantNeededDLI);
    
    currentPlannedDLI = (targetDLIAllocation * (hourlyDLI[startHour] / physicalDLILeft));
}

uint8_t LightController::findOptimumState() {
    uint8_t numStates = static_cast<uint8_t>(ServoController::ShadeID::ALL);
    if(!timeManager.withinPhotoperiod()) {
        Serial.println("System Alert: Outside active photoperiod. Holding blinds in dark mode until morning.");
        return numStates;
        // After this, stop running the algorithm and send a message to the user to put their plants in darkness.
        // Then, wait until the next time we hit timeManager.startHour.
    }

    updatePlannedHourlyDLI();

    double minCost = 0.0;
    uint8_t minState = 0;
    for(uint8_t i{0}; i < numStates; ++i){
        shades.setShades(i);
        delay(50);

        double cost = 0.0;

        int hour, minute, second;
        timeManager.getCurrentTime(hour, minute, second);
        int timeInHour = (minute * 60) + second;
        double planError = (currentPlannedDLI > 0.0) ? (dliTracker.getCurrentDLI() / currentPlannedDLI) - (timeInHour / 3600.0) : 0.0;
        if(abs(planError) < 0.03) planError = 0;
        double planWeight = 10 + 50 * abs(planError);

        double currentPPFDLevel = sensor.readPPFDLevel();
        double targetPPFD = plant.getTargetPPFD();
        double ppfdError = currentPPFDLevel - targetPPFD;
        double remainingDLI = weather.getRemainingDLI();

        if((planError > 0 && ppfdError > 0)
        || (planError < 0 && ppfdError < 0)){
            cost += planWeight * ppfdError * ppfdError;
        }

        cost += ppfdError * ppfdError;

        
        if((targetPPFD < 20)
        || (targetPPFD < 50 && currentPPFDLevel > 1.2 * targetPPFD)
        || (targetPPFD < 200 && currentPPFDLevel > 1.5 * targetPPFD)
        || (targetPPFD < 600 && currentPPFDLevel > 2 * targetPPFD)
        || (currentPPFDLevel > 2.5 * targetPPFD)){
            cost += 10000;
        }

        if(i == 0){
            minCost = cost;
        } else if(cost < minCost) {
            minCost = cost;
            minState = i;
        }
    }

    return minState;
}