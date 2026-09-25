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

// Sensor readiness flag
bool sensorReady = false;

// Enum to represent the state of the system
enum class SystemState {
  BLUETOOTH_SETUP,
  WAIT_FOR_WIFI,
  WAIT_FOR_TIME_SYNC,
  WAIT_FOR_WEATHER_UPDATE,
  READY
};
SystemState currentState = SystemState::BLUETOOTH_SETUP;

// Shared system state flags
bool scanInProgress = false;
bool pendingClose = false;

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

  // Check whether the light sensor initialized successfully.
  sensorReady = sensor.readPPFDLevel() > 0;

  if (currentPlant.isConfigured() && settings.isFullConfigured()) {
    currentState = SystemState::WAIT_FOR_WIFI;
  }
  else {
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

  // --- STATE MACHINE ---
  // Check the system state and perform actions accordingly
  switch (currentState) {

    case SystemState::BLUETOOTH_SETUP:
    {
      // Periodically print a status message to the console every 3 seconds
      static unsigned long lastBluetoothLog = 0;
      if (millis() - lastBluetoothLog > 3000UL) {
        Serial.println("System: Device unconfigured. Awaiting JSON payload from website...");
        lastBluetoothLog = millis();
      }

      // Check if the smartphone app has filled all the required slots
      if (currentPlant.isConfigured() && settings.isFullConfigured()) {
        Serial.println("System: Configuration received via Bluetooth!");
        Serial.println("System: Initializing WiFi connection...");

        // Start the background WiFi engine now that credentials exist
        network.begin();

        // Push the state machine forward!
        currentState = SystemState::WAIT_FOR_WIFI;
      }
      break;
    }

    case SystemState::WAIT_FOR_WIFI:
    {
      // Wait until WiFi is connected before attempting time synchronization.
      if (network.isConnected()) {
        Serial.println("System: WiFi connected. Waiting for time synchronization...");
        timeManager.begin();

        currentState = SystemState::WAIT_FOR_TIME_SYNC;
      }
      break;
    }

    case SystemState::WAIT_FOR_TIME_SYNC:
    {
      // Periodically attempts an NTP sync if the internal clock is uninitialized
      static unsigned long lastTimeSync = 0;

      if (timeManager.isTimeSynced()) {
        Serial.println("System: Time synchronized.");

        // The initial weather update is handled in WAIT_FOR_WEATHER_UPDATE.
        currentState = SystemState::WAIT_FOR_WEATHER_UPDATE;
      }
      else if (network.isConnected() && millis() - lastTimeSync > 1000UL) {
        Serial.println("System: Internal clock invalid. Re-attempting NTP Time Sync...");
        timeManager.begin();
        lastTimeSync = millis();
      }

      // If WiFi disconnects, return to the WiFi wait state.
      if (!network.isConnected()) {
        Serial.println("System: WiFi disconnected. Waiting for reconnection...");
        currentState = SystemState::WAIT_FOR_WIFI;
      }

      break;
    }

    case SystemState::WAIT_FOR_WEATHER_UPDATE:
    {
      // Do not proceed until the required sensor, WiFi, and time conditions are satisfied.
      if (!currentPlant.isConfigured() || !settings.isFullConfigured()) {
        currentState = SystemState::BLUETOOTH_SETUP;
        break;
      }

      if (!network.isConnected()) {
        Serial.println("System: WiFi disconnected. Waiting for reconnection...");
        currentState = SystemState::WAIT_FOR_WIFI;
        break;
      }

      if (!timeManager.isTimeSynced()) {
        Serial.println("System: Time synchronization lost. Waiting...");
        currentState = SystemState::WAIT_FOR_TIME_SYNC;
        break;
      }

      if (!sensorReady) {
        // Retry sensor initialization in case it was not ready at startup.
        sensor.begin();

        // Adapt this call to match the readiness method in your LightSensor class.
        sensorReady = sensor.readPPFDLevel() > 0;

        if (!sensorReady) {
          static unsigned long lastSensorLog = 0;

          if (millis() - lastSensorLog > 3000UL) {
            Serial.println("System: Light sensor is not ready. Retrying...");
            lastSensorLog = millis();
          }

          break;
        }
      }

      // Initial weather update
      // Retry periodically until a successful forecast update is received.
      static unsigned long lastWeatherAttempt = 0;

      if (millis() - lastWeatherAttempt > 30000UL || lastWeatherAttempt == 0) {
        if (network.isConnected() && timeManager.isTimeSynced()) {
          Serial.println("System: Performing initial weather forecast update...");

          // Requires weather.update() to return true on success and false on failure.
          bool weatherUpdated = weather.update();

          lastWeatherAttempt = millis();

          if (weatherUpdated) {
            Serial.println("System: Initial weather forecast received.");

            firstWeatherUpdate = false;

            // Initialize photoperiod history to avoid detecting a false transition at startup.
            wasInPhotoperiod = timeManager.withinPhotoperiod();

            currentState = SystemState::READY;
          }
          else {
            Serial.println("System: Initial weather update failed. Will retry.");
          }
        }
      }

      break;
    }

    case SystemState::READY:
    {
      // If a required service becomes unavailable, pause light-control operations
      // until the system is ready again.
      if (!currentPlant.isConfigured() || !settings.isFullConfigured()) {
        currentState = SystemState::BLUETOOTH_SETUP;
        break;
      }

      if (!network.isConnected()) {
        Serial.println("System: WiFi connection lost. Pausing light control...");
        currentState = SystemState::WAIT_FOR_WIFI;
        break;
      }

      if (!timeManager.isTimeSynced()) {
        Serial.println("System: Time synchronization lost. Pausing light control...");
        currentState = SystemState::WAIT_FOR_TIME_SYNC;
        break;
      }

      if (!sensorReady) {
        Serial.println("System: Light sensor unavailable. Pausing light control...");
        currentState = SystemState::WAIT_FOR_WEATHER_UPDATE;
        break;
      }

      // --- SYSTEM-READY UPDATES ---
      // Keep updating our currentDLI
      dliTracker.update();

      // --- TIME SYNC MANAGER ---
      // Periodically attempts an NTP sync if the internal clock is uninitialized
      static unsigned long lastTimeSync = 0;
      if (!timeManager.isTimeSynced() && millis() - lastTimeSync > 1000UL) {
        if (network.isConnected()) {
          Serial.println("System: Internal clock invalid. Re-attempting NTP Time Sync...");
          timeManager.begin();
          lastTimeSync = millis();
        }
      }

      // --- CLOUD WEATHER ENGINE (Every 5 minutes) ---
      static unsigned long lastWeatherUpdate = 0;

      // Do not start a periodic weather update while a scan is in progress.
      if ((millis() - lastWeatherUpdate > 300000UL || firstWeatherUpdate)
          && !scanInProgress) { // 5 mins
        if (network.isConnected() && timeManager.isTimeSynced() && timeManager.withinPhotoperiod()) {
          Serial.println("System: Updating weather forecast curves...");

          // Requires weather.update() to return true on success and false on failure.
          if (weather.update()) {
            lastWeatherUpdate = millis();
            firstWeatherUpdate = false;
          }
        }
      }

      // --- HARDWARE OPTIMIZATION STATE ---
      static unsigned long lastStateOptimize = 0;

      bool currentlyInPhotoperiod = timeManager.withinPhotoperiod();

      // A scan must never issue another movement after the photoperiod ends.
      // Let ServoController finish any movement already in progress, then close.
      if (!currentlyInPhotoperiod && lightControl.isCurrentlyOptimizing()) {
        lightControl.cancelOptimization();
      }

      // Advance the scan only while the photoperiod is active.
      if (currentlyInPhotoperiod) {
        lightControl.update();
      }

      // --- DETECT PHOTOPERIOD TRANSITIONS ---
      // Transition: inside -> outside the photoperiod.
      if (wasInPhotoperiod && !currentlyInPhotoperiod) {
        Serial.println("System Alert: Photoperiod ended.");

        Serial.print(timeManager.getHour());
        Serial.print(" is not within ");
        Serial.print(timeManager.getPhotoperiodStartTime());
        Serial.print(" and ");
        Serial.println(timeManager.getPhotoperiodEndTime());

        // Defer closing if a scan is still running.
        pendingClose = true;
      }

      // Transition: outside -> inside the photoperiod.
      if (!wasInPhotoperiod && currentlyInPhotoperiod) {
        Serial.println("System: Photoperiod started.");
        pendingClose = false;
      }

      // --- HANDLE SCAN COMPLETION ---
      // This check is independent of the current photoperiod.
      if (scanInProgress && !lightControl.isCurrentlyOptimizing()) {
        Serial.println("System: Physical shade scan complete.");

        scanInProgress = false;
        firstOptimization = false;

        // Start the rest interval after the scan has finished.
        lastStateOptimize = millis();
      }

      // --- HANDLE PENDING CLOSURE ---
      // Close only after any active scan has finished.
      if (!currentlyInPhotoperiod && pendingClose && !scanInProgress) {
        Serial.println("System: Closing shades until morning.");

        bool accepted = shades.setShades(ServoController::ShadeID::ALL, true);

        if (accepted) {
          dliTracker.reset();
          pendingClose = false;
        }
        else {
          Serial.println("System: Shade closure deferred; ServoController is busy.");
        }
      }

      // --- START A NEW OPTIMIZATION SCAN ---
      if (currentlyInPhotoperiod && !scanInProgress && !shades.isMoving() && !pendingClose
          && (firstOptimization || millis() - lastStateOptimize > 300000UL)) {
        Serial.println("System: Recalculating optimum window blind positioning...");

        scanInProgress = lightControl.optimizeState();
      }

      // --- UPDATE PHOTOPERIOD HISTORY ---
      wasInPhotoperiod = currentlyInPhotoperiod;

      break;
    }
  }
}