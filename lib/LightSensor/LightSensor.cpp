#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include "LightSensor.h"
#include "Pins.h"

BH1750 meter;

bool LightSensor::begin() {
    Wire.begin(LIGHT_SDA_PIN, LIGHT_SCL_PIN);
    return meter.begin();
}

float LightSensor::readLightLevel() {
    return meter.readLightLevel();
}