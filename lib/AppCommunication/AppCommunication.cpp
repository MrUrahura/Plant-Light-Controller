#include <Arduino.h>
#include "SettingsManager.h"

// Variable declarations
struct SetupData {
    Plant plant;
    String ssid;
    String password;
    String apiKey;
    double latitude;
    double longitude;
};

AppCommunication::AppCommunication()
{

}

void AppCommunication::update() {
    // Implement the logic to communicate with the app
}