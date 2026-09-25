#include <Arduino.h>
#include <Wire.h>
#include "LightSensor.h"
#include "Pins.h"

bool LightSensor::begin() {
    Wire.begin(LIGHT_SDA_PIN, LIGHT_SCL_PIN);
    Serial.println("Initializing BH1750...");

    bool success = meter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

    if (success) {
        if (!connected) {
            Serial.println("BH1750 OK");
        }
    } else {
        Serial.println("BH1750 FAILED");
    }

    connected = success;

    return success;
}

double LightSensor::readPPFDLevel() const {
    float lux = readLuxLevel();

    if(lux < 0) {
        Serial.println("LightSensor: Unexpected lux reading below 0.");
        return 0.0;
    }

    return static_cast<double>(lux) / 54.0; // Convert lux to PPFD (approximation)
}

// Private helper function to read the light level in lux
float LightSensor::readLuxLevel() const {
    // const_cast is needed because the library's readLightLevel() is not marked const
    return const_cast<BH1750&>(meter).readLightLevel();
}