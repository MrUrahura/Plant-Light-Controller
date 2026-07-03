#pragma once
#include <Arduino.h>

class Plant {
public:
    void setPlant(const String& name, const String& type, int minDLI, int maxDLI, int photoperiod);

    String getPlantName();    
    String getPlantType();    
    int getTargetDLI();
    int getMinDLI();
    int getMaxDLI();
    const int& getPhotoperiod();

private:
    String plantName;
    String plantType;
    int targetDLI;
    int minDLI;
    int maxDLI;
    int photoperiod;
};