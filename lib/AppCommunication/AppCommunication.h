#pragma once
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Plant.h"
#include "SettingsManager.h"
#include "ServoController.h"
#include "TimeManager.h"

class AppCommunication : public BLECharacteristicCallbacks {
public:
    AppCommunication(Plant& plant, SettingsManager& settings, ServoController& servos, TimeManager& timeManager);
    void begin();
    void update();
    void onWrite(BLECharacteristic* pCharacteristic) override;
    void onRead(BLECharacteristic* pCharacteristic) override;
    void updateBLEStatus();
    bool takeOptimizationRequest();
    void setControllerStatus(
        bool timeSynced,
        bool inPhotoperiod,
        bool optimizing,
        int32_t secondsUntilNextOptimization,
        int32_t secondsSincePreviousOptimization,
        uint8_t optimizationProgress,
        bool optimizationFailed,
        bool wifiConnected,
        bool weatherRead
    );

private:
    Plant& plant;
    SettingsManager& settings;
    ServoController& servos;
    TimeManager& timeManager;

    BLEServer* pServer = nullptr;
    BLECharacteristic* pCharacteristic = nullptr;
    volatile bool hasNewData = false;
    volatile bool optimizationRequested = false;
    bool controllerTimeSynced = false;
    bool controllerInPhotoperiod = false;
    bool controllerOptimizing = false;
    bool controllerWifiConnected = false;
    bool controllerWeatherRead = false;
    int32_t secondsUntilNextOptimization = -1;
    int32_t secondsSincePreviousOptimization = -1;
    uint8_t optimizationProgress = 0;
    bool optimizationFailed = false;
    String bleBuffer;
    bool receivingData = false;
};