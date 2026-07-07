#include <Arduino.h>
#include "Plant.h"
#include "LightSensor.h"
#include "ServoController.h"
#include "WeatherForecast.h"
#include "TimeManager.h"
#include "DLITracker.h"
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "AppCommunication.h"
#include "LightController.h"

// Components declarations
Plant currentPlant;
LightSensor sensor;
ServoController shades;
SettingsManager settings;
DLITracker dliTracker(sensor);
NetworkManager network(settings);
AppCommunication appComm(currentPlant, settings);
TimeManager timeManager(settings, network, currentPlant.getPhotoperiod());
WeatherForecast weather(settings, network, timeManager);
LightController lightControl(currentPlant, sensor, shades, dliTracker, weather, timeManager);

// Enum to represent the state of the system
enum class SystemState {
  BLUETOOTH_SETUP,
  CONNECTING_WIFI,
  SYNCING_TIME,
  FETCHING_WEATHER,
  READY
};
SystemState currentState = SystemState::BLUETOOTH_SETUP;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize components along with their "begin" or "load" method
  settings.load();
  network.begin();

  if (settings.isFullConfigured())
  {
    currentState = SystemState::CONNECTING_WIFI;
  }
  else
  {
    currentState = SystemState::BLUETOOTH_SETUP;
  }

  Serial.println("System initialized.");
}

void loop() {
  network.handle(); // Handle network reconnections if needed

  // Check the system state and perform actions accordingly
  switch (currentState) {
    case SystemState::BLUETOOTH_SETUP:
      Serial.println("Waiting for Bluetooth setup...");
      // Handled when user configures the device via Bluetooth
      currentState = SystemState::CONNECTING_WIFI;
      break;

    case SystemState::CONNECTING_WIFI:
      Serial.println("Connecting to WiFi...");
      // Attempt to connect to wifi (maybe not necessary due to handle() in loop, but ensures connection attempt)
      network.connectWiFi();
      if (network.isConnected()) {
        Serial.println("WiFi connected.");
        currentState = SystemState::SYNCING_TIME;
      }
      break;

    case SystemState::SYNCING_TIME:
      Serial.println("Syncing time...");
      // Call TimeManager's begin function to sync time
      timeManager.begin();
      currentState = SystemState::FETCHING_WEATHER;
      break;

    case SystemState::FETCHING_WEATHER:
      Serial.println("Fetching weather data...");
      // Call WeatherForecast's update function to fetch weather data
      weather.update();
      currentState = SystemState::READY;
      break;

    case SystemState::READY:
      Serial.println("System is ready.");
      // Perform regular operations like reading sensors and controlling blinds
      break;
  }

  delay(1000);
}