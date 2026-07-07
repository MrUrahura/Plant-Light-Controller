#include <Arduino.h>
#include <Wire.h>
#include "LightSensor.h"
#include "Pins.h"

bool LightSensor::begin() {
    Wire.begin(LIGHT_SDA_PIN, LIGHT_SCL_PIN);
    return meter.begin();
}

double LightSensor::readPPFDLevel() const {
    float lux = readLuxLevel();
    return static_cast<double>(lux) / 54.0; // Convert lux to PPFD (approximation)
}

// Private helper function to read the light level in lux
float LightSensor::readLuxLevel() const {
    return meter.readLightLevel();
}