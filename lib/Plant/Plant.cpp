#include <Arduino.h>
#include "Plant.h"

void Plant::setPlant(const String& name, const String& type, int minDLI, int maxDLI, int photoperiod) {
    this->name = name;
    this->type = type;
    this->targetDLI = (minDLI + maxDLI) / 2.0; // Calculate target DLI as the average of min and max
    this->minDLI = minDLI;
    this->maxDLI = maxDLI;
    this->photoperiod = photoperiod;
    this->targetPPFD = targetDLI * 1000000.0 / (photoperiod * 3600.0);
}

const String& Plant::getName() const {
    return name;
}

const String& Plant::getType() const {
    return type;
}

double Plant::getTargetDLI() const {
    return targetDLI;
}

int Plant::getMinDLI() const {
    return minDLI;
}

int Plant::getMaxDLI() const {
    return maxDLI;
}

const int& Plant::getPhotoperiod() const {
    return photoperiod;
}

double Plant::getTargetPPFD() const {
    return targetPPFD;
}