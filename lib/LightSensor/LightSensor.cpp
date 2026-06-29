#include "LightSensor.h"
#include "Pins.h"
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

bool LightSensor::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    return meter.begin();
}