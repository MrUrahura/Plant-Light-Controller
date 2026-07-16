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
NetworkManager network(settings);
AppCommunication appComm(currentPlant, settings);
TimeManager timeManager(settings, network, currentPlant.getPhotoperiod());
DLITracker dliTracker(sensor, timeManager);
WeatherForecast weather(settings, network, timeManager);
LightController lightControl(currentPlant, sensor, shades, dliTracker, weather, timeManager);

// Enum to represent the state of the system
enum class SystemState {
  BLUETOOTH_SETUP,
  READY
};
SystemState currentState = SystemState::BLUETOOTH_SETUP;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Test the hardware and its calibration
  //testBlock();

  // Initialize components along with their "begin" or "load" method
  currentPlant.loadPlant();
  settings.load();
  sensor.begin();
  shades.begin();
  dliTracker.loadDLI();
  appComm.begin();

  if (currentPlant.isConfigured() && settings.isFullConfigured())
  {
    network.begin();
    currentState = SystemState::READY;
  }
  else
  {
    currentState = SystemState::BLUETOOTH_SETUP;
  }

  Serial.println("System initialized.");
}

void loop() {
  // --- GLOBAL SYSTEM LISTENERS ---
  // Keep background network management always running
  network.handle();
  // Keep the Bluetooth interface with the app up to listen for changes from the app
  appComm.update();

  // --- STATE MACHINE ---
  // Check the system state and perform actions accordingly
  switch (currentState) {
    case SystemState::BLUETOOTH_SETUP:
      // Periodically print a status message to the console every 3 seconds
      static unsigned long lastBluetoothLog = 0;
      if (millis() - lastBluetoothLog > 3000) {
          Serial.println("System: Device unconfigured. Awaiting JSON payload from website...");
          lastBluetoothLog = millis();
      }

      // Check if the smartphone app has filled all the required slots
      if (currentPlant.isConfigured() && settings.isFullConfigured()) {
          Serial.println("System: Configuration received via Bluetooth!");
          Serial.println("System: Initializing WiFi connection and transitioning to READY...");
          
          // Start the background WiFi engine now that credentials exist
          network.begin(); 
          
          // Push the state machine forward!
          currentState = SystemState::READY; 
      }
      break;

    case SystemState::READY:
      // --- TIME SYNC MANAGER ---
      // Periodically attempts an NTP sync if the internal clock is uninitialized
      static unsigned long lastTimeSync = 0;
      if (!timeManager.isTimeSynced() && millis() - lastTimeSync > 10000) {
          if (network.isConnected()) {
              Serial.println("System: Internal clock invalid. Re-attempting NTP Time Sync...");
              timeManager.begin();
              lastTimeSync = millis();
          }
      }

      // --- CLOUD WEATHER ENGINE (Every 15 minutes) ---
      static unsigned long lastWeatherUpdate = 0;
      if (millis() - lastWeatherUpdate > 900000) { // 15 mins
          if (network.isConnected() && timeManager.isTimeSynced()) {
              Serial.println("System: Updating weather forecast curves...");
              weather.update();
              lastWeatherUpdate = millis();
          }
      } 

      // --- HARDWARE OPTIMIZATION CALCULATOR (Every 5 minutes) ---
      static unsigned long lastStateOptimize = 0;
      if (millis() - lastStateOptimize > 300000) { // 5 mins
          if (timeManager.isTimeSynced()) {
            Serial.println("System: Recalculating optimum window blind positioning...");
            lightControl.optimizeState();
          }
          lastStateOptimize = millis();
      }
      break;
  }
}

void testBlock(){
  // Run some tests to ensure the hardware works correctly
  Serial.println("Running tests...");
  Serial.println(sensor.readPPFDLevel());
  Serial.println(shades.getCurrentState());
  shades.setShades(ServoController::ShadeID::NONE);
  Serial.println(shades.getCurrentState());
  shades.setShades(ServoController::ShadeID::ALL);
  delay(1000);
}