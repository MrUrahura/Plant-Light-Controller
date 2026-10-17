#pragma once
#include <Arduino.h>
#include <Preferences.h>

class Plant {
public:
    void setPlant(
        const String& name,
        const String& type,
        const String& stage,
        int minDLI,
        int maxDLI,
        int photoperiod
    );
    void loadPlant();

    const String& getName() const;
    const String& getType() const;
    const String& getStage() const;
    double getTargetDLI() const;
    int getMinDLI() const;
    int getMaxDLI() const;
    const int& getPhotoperiod() const;
    double getTargetPPFD() const; 
    bool isConfigured() const;

private:
    String name;
    String type;
    String stage;
    double targetDLI;
    int minDLI;
    int maxDLI;
    int photoperiod;
    double targetPPFD;
    Preferences prefs;
};