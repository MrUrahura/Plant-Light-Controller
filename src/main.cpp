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
TimeManager timeManager(settings, network, currentPlant);
DLITracker dliTracker(sensor, timeManager);
WeatherForecast weather(settings, network, timeManager);
LightController lightControl(currentPlant, sensor, shades, dliTracker, weather, timeManager);

// First run flags
bool firstWeatherUpdate = true;
bool firstOptimization = true;
bool wasInPhotoperiod = false;

// Enum to represent the state of the system
enum class SystemState {
  BLUETOOTH_SETUP,
  READY
};
SystemState currentState = SystemState::BLUETOOTH_SETUP;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

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
    timeManager.begin();

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
  // Non-blocking shades operation so the shades can be checked each time in case they're moving
  shades.update();
  // Non-blocking light controlling operation to keep the checking also non-blocking along with the shades
  lightControl.update();

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
          timeManager.begin();

          // Push the state machine forward!
          currentState = SystemState::READY; 
      }
      break;

    case SystemState::READY:
      // --- SYSTEM-READY UPDATES ---
      // Keep updating our currentDLI
      dliTracker.update();  
    
      // --- TIME SYNC MANAGER ---
      // Periodically attempts an NTP sync if the internal clock is uninitialized
      static unsigned long lastTimeSync = 0;
      if (!timeManager.isTimeSynced() && millis() - lastTimeSync > 1000) {
          if (network.isConnected()) {
              Serial.println("System: Internal clock invalid. Re-attempting NTP Time Sync...");
              timeManager.begin();
              lastTimeSync = millis();
          }
      }

      // --- CLOUD WEATHER ENGINE (Every 5 minutes) ---
      static unsigned long lastWeatherUpdate = 0;
      if (millis() - lastWeatherUpdate > 300000 || firstWeatherUpdate) { // 5 mins
          if (network.isConnected() && timeManager.isTimeSynced() && timeManager.withinPhotoperiod()) {
              Serial.println("System: Updating weather forecast curves...");
              weather.update();
              lastWeatherUpdate = millis();
              firstWeatherUpdate = false;
          }
      } 

      // --- HARDWARE OPTIMIZATION CALCULATOR (Every 5 minutes) ---
      static unsigned long lastStateOptimize = 0;
      static bool scanInProgress = false;
      bool currentlyInPhotoperiod = timeManager.withinPhotoperiod();

      if (millis() - lastStateOptimize > 300000 || firstOptimization || scanInProgress) { // 5 mins

          // Transition: inside -> outside (leaving photoperiod)
          if (wasInPhotoperiod && !currentlyInPhotoperiod && !scanInProgress) {
              Serial.println("System Alert: Photoperiod ended. Closing shades until morning.");

              Serial.print(timeManager.getHour());
              Serial.print(" is not within ");
              Serial.print(timeManager.getPhotoperiodStartTime());
              Serial.print(" and ");
              Serial.print(timeManager.getPhotoperiodEndTime());

              shades.setShades(ServoController::ShadeID::ALL);
              dliTracker.reset();
          }

          // Transition: outside -> inside (entering photoperiod)
          if (!wasInPhotoperiod && currentlyInPhotoperiod && !scanInProgress) {
              Serial.println("System: Photoperiod started. Beginning light optimization.");
          }

          // Handle Active Scanning Step Matrix Execution
          if (currentlyInPhotoperiod) {
            if (!scanInProgress) {
              Serial.println("System: Recalculating optimum window blind positioning...");
              lightControl.optimizeState();
              scanInProgress = true;
            }
            else if (!lightControl.isCurrentlyOptimizing()){
                Serial.println("System: Physical shade scan complete. Beginning 5-minute rest cycle.");
                scanInProgress = false;
                firstOptimization = false;
                wasInPhotoperiod = currentlyInPhotoperiod;
                lastStateOptimize = millis(); // Countdown officially starts NOW, since the scan is complete and we are now in a 5-minute rest cycle
            }
          }

          // Only track edge history variables if an active scan sequence isn't overriding configurations
          if(!scanInProgress) {
            firstOptimization = false;
            wasInPhotoperiod = currentlyInPhotoperiod;
            if (millis() - lastStateOptimize > 300000) {
                lastStateOptimize = millis();
            }
          }
      }
      break;
  }
}