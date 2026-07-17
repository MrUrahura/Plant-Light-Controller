#pragma once
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Plant.h"
#include "SettingsManager.h"

class AppCommunication : public BLECharacteristicCallbacks {
public:
    AppCommunication(Plant& plant, SettingsManager& settings);
    void begin();
    void update();
    void onWrite(BLECharacteristic* pCharacteristic) override;
    void onRead(BLECharacteristic* pCharacteristic);
    void updateBLEStatus();

private:
    Plant& plant;
    SettingsManager& settings;

    BLEServer* pServer = nullptr;
    BLECharacteristic* pCharacteristic = nullptr;
    bool hasNewData = false;
    String bleBuffer;
    bool receivingData = false;
};