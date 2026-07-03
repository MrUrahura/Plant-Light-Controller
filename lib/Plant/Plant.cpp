#include <Arduino.h>
#include "Plant.h"

void Plant::setPlant(const String& name, const String& type, int minDLI, int maxDLI, int photoperiod) {
    this->name = name;
    this->type = type;
    this->targetDLI = (minDLI + maxDLI) / 2; // Calculate target DLI as the average of min and max
    this->minDLI = minDLI;
    this->maxDLI = maxDLI;
    this->photoperiod = photoperiod;
}