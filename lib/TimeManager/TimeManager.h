#pragma once
#include <Arduino.h>
#include "SettingsManager.h"
#include "NetworkManager.h"

class TimeManager {
public:
    TimeManager(const SettingsManager& settings, const NetworkManager& network, const int& photoperiod);
    void begin();
    void getCurrentTime(int &hour, int &minute, int &second) const;
    void getCurrentDate(int &year, int &month, int &day) const;
    int getHour() const;
    int getDayOfYear() const;
    int getPhotoperiodEndTime() const;
    String getCurrentDateString() const;
    bool withinPhotoperiod() const;

private:
    const SettingsManager& settings;
    const NetworkManager& network;
    const int& photoperiod;
};