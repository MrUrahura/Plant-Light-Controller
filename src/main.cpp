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
unsigned long timeSyncCompletedAt = 0;
unsigned long lastWeatherAttempt = 0;

// Constant times
const unsigned long BLUETOOTH_LOG_INTERVAL = 3000UL;
const unsigned long TIME_SYNC_RETRY_INTERVAL = 1000UL;
const unsigned long SENSOR_CHECK_INTERVAL = 3000UL;
const unsigned long INITIAL_WEATHER_RETRY_INTERVAL = 10000UL;
const unsigned long WEATHER_UPDATE_CHECK_INTERVAL = 1800000UL;
const unsigned long OPTIMIZATION_CHECK_INTERVAL = 1800000UL;
const unsigned long INITIAL_OPTIMIZATION_RETRY_INTERVAL = 30000UL;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize components along with their "begin" or "load" method
  currentPlant.loadPlant();
  settings.load();
  double initialPPFD;
  sensorReady = sensor.begin() && sensor.tryReadPPFDLevel(initialPPFD);
  shades.begin();
  dliTracker.loadDLI();
  appComm.begin();

  if (currentPlant.isConfigured() && settings.isFullConfigured()) {
    network.begin();
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
      if (millis() - lastBluetoothLog > BLUETOOTH_LOG_INTERVAL) {
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
        timeSyncCompletedAt = millis();

        // The initial weather update is handled in WAIT_FOR_WEATHER_UPDATE.
        currentState = SystemState::WAIT_FOR_WEATHER_UPDATE;
      }
      else if (network.isConnected() && millis() - lastTimeSync > TIME_SYNC_RETRY_INTERVAL) {
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

      // Give the WiFi/BLE coexistence scheduler time to settle after NTP completes before opening the first TLS connection.
      if (millis() - timeSyncCompletedAt < BLUETOOTH_LOG_INTERVAL) {
        break;
      }

      // Outside the photoperiod, defer sensor readiness retries and weather
      // retrieval until the photoperiod starts.
      if (!timeManager.withinPhotoperiod()) {
        wasInPhotoperiod = false;
        currentState = SystemState::READY;
        break;
      }

      if (!sensorReady) {
        static unsigned long lastSensorCheck = 0;
        unsigned long now = millis();
        if (now - lastSensorCheck < SENSOR_CHECK_INTERVAL) {
          break;
        }
        lastSensorCheck = now;

        // Retry sensor initialization periodically in case it was not ready at startup.
        double ppfd;
        sensorReady = sensor.begin() && sensor.tryReadPPFDLevel(ppfd);

        if (!sensorReady) {
          Serial.println("System: Light sensor is not ready. Retrying...");
          break;
        }
      }

      // Initial weather update
      // Retry periodically until a successful forecast update is received.
      if (millis() - lastWeatherAttempt > INITIAL_WEATHER_RETRY_INTERVAL || lastWeatherAttempt == 0) {
        if (network.isConnected() && timeManager.isTimeSynced()) {
          Serial.println("System: Performing initial weather forecast update...");

          // Requires weather.update() to return true on success and false on failure.
          bool weatherUpdated = weather.update();
          lastWeatherAttempt = millis();

          if (weatherUpdated) {
            Serial.println("System: Initial weather forecast received.");

            firstWeatherUpdate = false;
          }
          else {
            Serial.println("System: Initial weather update failed. Continuing light control and retrying.");
          }

          // A missing forecast should not prevent the initial physical scan.
          // LightController uses its PPFD target when no planned DLI is available.
          wasInPhotoperiod = timeManager.withinPhotoperiod();
          currentState = SystemState::READY;
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

      if (!timeManager.isTimeSynced()) {
        Serial.println("System: Time synchronization lost. Pausing light control...");
        currentState = SystemState::WAIT_FOR_TIME_SYNC;
        break;
      }

      // Once time has been synchronized, shade optimization is local and can
      // continue during temporary WiFi outages. Weather updates remain gated
      // on connectivity below.
      bool currentlyInPhotoperiod = timeManager.withinPhotoperiod();

      if (!sensorReady && currentlyInPhotoperiod) {
        Serial.println("System: Light sensor unavailable. Pausing light control...");
        currentState = SystemState::WAIT_FOR_WEATHER_UPDATE;
        break;
      }

      // --- SYSTEM-READY UPDATES ---
      // Keep updating our currentDLI
      dliTracker.update();

      // --- CLOUD WEATHER ENGINE (Every 5 minutes) ---
      // Do not start a periodic weather update while a scan is in progress.
      unsigned long weatherRetryInterval = firstWeatherUpdate
          ? INITIAL_WEATHER_RETRY_INTERVAL
          : WEATHER_UPDATE_CHECK_INTERVAL;
      if ((lastWeatherAttempt == 0 || millis() - lastWeatherAttempt > weatherRetryInterval)
          && !scanInProgress && currentlyInPhotoperiod) {
        if (network.isConnected() && timeManager.isTimeSynced()) {
          Serial.println("System: Updating weather forecast curves...");

          // Requires weather.update() to return true on success and false on failure.
          bool weatherUpdated = weather.update();
          lastWeatherAttempt = millis();

          if (weatherUpdated) {
            firstWeatherUpdate = false;
          }
        }
      }

      // --- HARDWARE OPTIMIZATION STATE ---
      static unsigned long lastStateOptimize = 0;

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
        if (lightControl.optimizationSucceeded()) {
          firstOptimization = false;
        }

        // Start the retry/rest interval after the scan has finished.
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
      bool optimizationDue = firstOptimization
          ? (lastStateOptimize == 0
              || millis() - lastStateOptimize > INITIAL_OPTIMIZATION_RETRY_INTERVAL)
          : millis() - lastStateOptimize > OPTIMIZATION_CHECK_INTERVAL;
      if (currentlyInPhotoperiod && !scanInProgress && !shades.isMoving() && !pendingClose
          && optimizationDue) {
        Serial.println("System: Recalculating optimum window blind positioning...");

        scanInProgress = lightControl.optimizeState();
      }

      // --- UPDATE PHOTOPERIOD HISTORY ---
      wasInPhotoperiod = currentlyInPhotoperiod;

      break;
    }
  }
}