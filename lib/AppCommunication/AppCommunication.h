#pragma once
#include "Plant.h"
#include "SettingsManager.h"

class AppCommunication {
public:
    AppCommunication(Plant& plant, SettingsManager& settings);
    void update();

private:
    Plant& plant;
    SettingsManager& settings;
};