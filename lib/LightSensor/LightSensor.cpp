#include <Arduino.h>
#include <Wire.h>
#include "LightSensor.h"
#include "Pins.h"

bool LightSensor::begin() {
    Wire.begin(LIGHT_SDA_PIN, LIGHT_SCL_PIN);
    return meter.begin();
}

float LightSensor::readPPFDLevel() {
    float lux = readLuxLevel();
    return lux / 54.0; // Convert lux to PPFD (approximation)
}

// Private helper function to read the light level in lux
float LightSensor::readLuxLevel() {
    return meter.readLightLevel();
}