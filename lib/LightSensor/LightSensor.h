#pragma once
#include <BH1750.h>

class LightSensor {
public:
    bool begin();
    float readPPFDLevel();

private:
    BH1750 meter;
    float readLuxLevel();
};