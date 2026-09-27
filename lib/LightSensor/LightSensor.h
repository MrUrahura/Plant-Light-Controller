#pragma once
#include <BH1750.h>

class LightSensor {
public:
    bool begin();
    bool tryReadPPFDLevel(double& ppfd) const;
    double readPPFDLevel () const;

private:
    BH1750 meter;
    bool connected = false;
    float readLuxLevel() const;
};