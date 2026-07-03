#pragma once
#include "SettingsManager.h"

class AppCommunication {
public:
    AppCommunication();
    void update();

private:
    const SettingsManager& settingsManager;
};