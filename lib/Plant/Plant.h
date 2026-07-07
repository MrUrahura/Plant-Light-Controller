#pragma once
#include <Arduino.h>

class Plant {
public:
    void setPlant(const String& name, const String& type, int minDLI, int maxDLI, int photoperiod);

    const String& getName() const;
    const String& getType() const;
    double getTargetDLI() const;
    int getMinDLI() const;
    int getMaxDLI() const;
    const int& getPhotoperiod() const;
    double getTargetPPFD() const; 

private:
    String name;
    String type;
    double targetDLI;
    int minDLI;
    int maxDLI;
    int photoperiod;
    double targetPPFD;
};