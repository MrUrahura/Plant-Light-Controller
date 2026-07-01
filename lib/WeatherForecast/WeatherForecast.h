#pragma once

class WeatherForecast {
public:
    bool update();

    float getPredictedRemainingDLI();
    float getPredictedAvailablePPFD();
    float getCloudCover();
};