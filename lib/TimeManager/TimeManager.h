#pragma once
#include <Arduino.h>
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "Plant.h"

class TimeManager {
public:
    TimeManager(const SettingsManager& settings, const NetworkManager& network, const Plant& plant);
    void begin();
    bool isTimeSynced() const;
    void getCurrentTime(int &hour, int &minute, int &second) const;
    void getCurrentDate(int &year, int &month, int &day) const;
    int getHour() const;
    int getDayOfYear() const;
    int getPhotoperiodStartTime() const;
    int getPhotoperiodEndTime() const;
    String getCurrentDateString() const;
    bool withinPhotoperiod() const;
    time_t getUnixTime() const;

private:
    const SettingsManager& settings;
    const NetworkManager& network;
    const Plant& plant;
};