#include <Arduino.h>
#include "AppCommunication.h"

struct SetupData {
    Plant plant;
    String ssid;
    String password;
    String apiKey;
    double latitude;
    double longitude;
    String timeZoneString;
    int startHour;
};

AppCommunication::AppCommunication(Plant& plant, SettingsManager& settings)
    : plant(plant), settings(settings)
{
    
}

void AppCommunication::update() {
    // Implement the logic to communicate with the app
}