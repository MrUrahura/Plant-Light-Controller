#include <Arduino.h>
#include <ArduinoJson.h>
#include "AppCommunication.h"

#define SERVICE_UUID "3ffb09d6-6ba2-4d32-bc92-cbdaf9798ffa"
#define CHARACTERISTIC_UUID "0dddf571-257c-4021-964b-a6fd9212384f"

class ServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        Serial.println("BLE client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("BLE client disconnected");
        BLEDevice::startAdvertising();
        Serial.println("BLE advertising restarted");
    }
};

AppCommunication::AppCommunication(
    Plant& plant,
    SettingsManager& settings,
    ServoController& servos,
    TimeManager& timeManager
)
    : plant(plant), settings(settings), servos(servos), timeManager(timeManager)
{
    
}

void AppCommunication::begin() {
    Serial.println("BLE: Initializing Bluetooth Server...");
    
    // Initialize the ESP32 Bluetooth Radio
    BLEDevice::init("Smart_Plant_Shade");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create the custom communication channel
    BLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    // CRUCIAL: Point Bluetooth events directly to this class instance
    pCharacteristic->setCallbacks(this);

    // Start broadcasting so the smartphone app can see the device
    pService->start();
    updateBLEStatus();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  
    BLEDevice::startAdvertising();
    
    Serial.println("BLE: Server is broadcasting. Waiting for site pairing...");
}

// Triggered automatically on a background thread when the phone sends data
void AppCommunication::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
        String chunk = String(value.c_str());

        Serial.println("BLE data received.");

        bleBuffer += chunk;

        if (bleBuffer.length() > 4096) {
            bleBuffer = "";
            Serial.println("BLE buffer overflow cleared");
            return;
        }

        // Check if this looks like the end of the JSON message
        if (chunk.endsWith("}")) {
            hasNewData = true;
            receivingData = false;
            Serial.println("BLE message complete");
        }
        else {
            receivingData = true;
        }
    }
}

void AppCommunication::onRead(BLECharacteristic* pCharacteristic) {
    updateBLEStatus();
    Serial.println("BLE read request handled.");
}

void AppCommunication::update() {
    // --- BLUETOOTH RECEPTION ---
    // Exit instantly 99.9% of the time (Zero CPU waste)
    if (!hasNewData) return;

    hasNewData = false; // Reset flag
    String payload = bleBuffer;
    bleBuffer = "";

    Serial.println("BLE configuration received.");

    // --- JSON SETUP ---
    // Allocate JSON memory block on the stack
    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("BLE JSON Parsing Error: ");
        Serial.println(error.c_str());
        return;
    }

    // --- NETWORK & REGIONAL CONFIGURATION SECTOR ---
    // If the phone passed network variables, extract and push to SettingsManager
    if (doc["ssid"].is<const char*>() && doc["pass"].is<const char*>()) {
        String ssid = doc["ssid"].as<String>();
        String pass = doc["pass"].as<String>();
        settings.setWiFi(ssid, pass);
    }
    if (doc["apiKey"].is<const char*>()) {
        settings.setAPIKey(doc["apiKey"].as<String>());
    }
    if (doc["startHour"].is<int>()) {
        settings.setStartHour(doc["startHour"].as<int>());
    }
    if (doc["lat"].is<double>() && doc["lng"].is<double>()) {
        settings.setLocation(doc["lat"].as<double>(), doc["lng"].as<double>());
    }
    if (doc["tz"].is<const char*>()) {
        settings.setTimeZoneString(doc["tz"].as<String>());
    }

    if (doc["servoTopOpenMs"].is<uint32_t>() &&
        doc["servoTopCloseMs"].is<uint32_t>() &&
        doc["servoLeftOpenMs"].is<uint32_t>() &&
        doc["servoLeftCloseMs"].is<uint32_t>() &&
        doc["servoRightOpenMs"].is<uint32_t>() &&
        doc["servoRightCloseMs"].is<uint32_t>()) {
        if (!servos.setMovementTimes(
                doc["servoTopOpenMs"].as<uint32_t>(),
                doc["servoTopCloseMs"].as<uint32_t>(),
                doc["servoLeftOpenMs"].as<uint32_t>(),
                doc["servoLeftCloseMs"].as<uint32_t>(),
                doc["servoRightOpenMs"].as<uint32_t>(),
                doc["servoRightCloseMs"].as<uint32_t>())) {
            Serial.println("BLE: Servo movement time update was rejected.");
        }
    }

    if (doc["optimizationIntervalMinutes"].is<int>() &&
        !timeManager.setOptimizationIntervalMinutes(doc["optimizationIntervalMinutes"].as<int>())) {
        Serial.println("BLE: Optimization schedule update was rejected.");
    }

    if (doc["optimizeNow"].is<bool>() && doc["optimizeNow"].as<bool>()) {
        optimizationRequested = true;
        Serial.println("BLE: Manual optimization requested.");
    }


    // --- PLANT BIOLOGY CONFIGURATION SECTOR ---
    // Check if the phone app passed plant profile settings
    if (doc["pName"].is<const char*>() && doc["photo"].is<int>()) {
        String pName = doc["pName"].as<String>();
        String pType = doc["pType"] | "Generic"; // Fallback text if type is skipped
        int minDLI = doc["minDLI"] | 10;
        int maxDLI = doc["maxDLI"] | 20;
        int photoperiod = doc["photo"].as<int>();

        // Push directly into your live Plant reference
        plant.setPlant(pName, pType, minDLI, maxDLI, photoperiod);
    }

    Serial.println("BLE Sync Complete: Settings and Plant definitions updated successfully!");
}

void AppCommunication::updateBLEStatus() {
    JsonDocument doc;

    doc["ssid"] = settings.getSSID();
    doc["pass"] = settings.getPassword();
    doc["apiKey"] = settings.getAPIKey();
    doc["startHour"] = settings.getStartHour();
    doc["lat"] = settings.getLatitude();
    doc["lng"] = settings.getLongitude();
    doc["tz"] = settings.getTimeZoneString();

    doc["pName"] = plant.getName();
    doc["pType"] = plant.getType();
    doc["minDLI"] = plant.getMinDLI();
    doc["maxDLI"] = plant.getMaxDLI();
    doc["photo"] = plant.getPhotoperiod();

    doc["servoTopOpenMs"] = servos.getTopOpenTimeMs();
    doc["servoTopCloseMs"] = servos.getTopCloseTimeMs();
    doc["servoLeftOpenMs"] = servos.getLeftOpenTimeMs();
    doc["servoLeftCloseMs"] = servos.getLeftCloseTimeMs();
    doc["servoRightOpenMs"] = servos.getRightOpenTimeMs();
    doc["servoRightCloseMs"] = servos.getRightCloseTimeMs();
    doc["optimizationIntervalMinutes"] = timeManager.getOptimizationIntervalMinutes();
    doc["timeSynced"] = controllerTimeSynced;
    doc["inPhotoperiod"] = controllerInPhotoperiod;
    doc["optimizing"] = controllerOptimizing;
    doc["secondsUntilNextOptimization"] = secondsUntilNextOptimization;
    doc["secondsSincePreviousOptimization"] = secondsSincePreviousOptimization;

    String output;
    serializeJson(doc, output);

    pCharacteristic->setValue(output.c_str());

    Serial.println("BLE status updated.");
}

bool AppCommunication::takeOptimizationRequest() {
    const bool requested = optimizationRequested;
    optimizationRequested = false;
    return requested;
}

void AppCommunication::setControllerStatus(
    bool timeSynced,
    bool inPhotoperiod,
    bool optimizing,
    int32_t secondsUntilNext,
    int32_t secondsSincePrevious
) {
    controllerTimeSynced = timeSynced;
    controllerInPhotoperiod = inPhotoperiod;
    controllerOptimizing = optimizing;
    secondsUntilNextOptimization = secondsUntilNext;
    secondsSincePreviousOptimization = secondsSincePrevious;
}