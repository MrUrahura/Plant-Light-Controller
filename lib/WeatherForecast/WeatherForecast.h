#pragma once
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "TimeManager.h"

class WeatherForecast {
public:
    WeatherForecast(const SettingsManager& settings, const NetworkManager& network, const TimeManager& timeManager);
    bool update();
    float getRemainingDLI();

private:
    const SettingsManager& settings;
    const NetworkManager& network;
    const TimeManager& timeManager;
    double totalDLI;
    double remainingDLI;
    void calculateRemainingDLI(int startHour, int endHour);
    double calculateClearSkyGHI(double latitude, int dayOfYear, int hour);
};