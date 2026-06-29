#pragma once

class LightSensor {
public:
    bool begin();
    float readLightLevel();
};