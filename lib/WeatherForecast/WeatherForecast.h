#pragma once
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "TimeManager.h"

class WeatherForecast {
public:
    WeatherForecast(const SettingsManager& settings, NetworkManager& network, const TimeManager& timeManager);
    void update();
    double getCompleteDayDLI() const;
    double getPhotoperiodDLI() const;
    double getRemainingDLI() const;
    const std::array<double, 24>& getHourlyDLI() const;

private:
    const SettingsManager& settings;
    NetworkManager& network;
    const TimeManager& timeManager;
    double completeDayDLI;
    double photoperiodDLI;
    double remainingDLI;
    std::array<double, 24> hourlyDLI;
    void calculateRemainingDLI();
    double calculateClearSkyGHI(int hour) const;
};