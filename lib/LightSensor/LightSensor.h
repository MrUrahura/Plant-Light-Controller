#pragma once
#include <BH1750.h>

class LightSensor {
public:
    bool begin();
    double readPPFDLevel () const;

private:
    BH1750& meter;
    float readLuxLevel() const;
};