#include <Arduino.h>
#include <WiFi.h>
#include "NetworkManager.h"
#include "SettingsManager.h"

NetworkManager::NetworkManager(const SettingsManager& settings)
    : settings(settings) {  }

void NetworkManager::begin(){
    WiFi.mode(WIFI_STA);
    // Wi-Fi modem sleep must remain enabled while Bluetooth is active.
    WiFi.setSleep(true);
    connectWiFi();
}

bool NetworkManager::isConfigured() const {
    return settings.isNetworkConfigured();
}

void NetworkManager::connectWiFi() {
    if (!isConfigured()) {
        Serial.println("NetworkManager: WiFi credentials or API key not configured.");
        return;
    }

    if (isConnected() || connectionAttemptInProgress) {
        return;
    }

    Serial.printf("NetworkManager: Initiating connection to %s...\n", settings.getSSID().c_str());

    WiFi.begin(settings.getSSID().c_str(), settings.getPassword().c_str());
    lastReconnectAttempt = millis();
    connectionAttemptInProgress = true;
    Serial.println("NetworkManager: Connection attempt started.");
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::handle() {
    if (isConnected()) {
        if (connectionAttemptInProgress) {
            Serial.println("NetworkManager: WiFi connected.");
            connectionAttemptInProgress = false;
        }
        return;
    }

    if (!isConfigured()) {
        return;
    }

    const unsigned long currentMillis = millis();
    if (connectionAttemptInProgress) {
        if (currentMillis - lastReconnectAttempt >= connectionTimeout) {
            const int status = WiFi.status();
            Serial.print("NetworkManager: Connection attempt timed out. Status: ");
            Serial.print(status);
            if (status == WL_DISCONNECTED) {
                Serial.print(" (WL_DISCONNECTED)");
            }
            Serial.println(".");
            WiFi.disconnect(false, false);
            connectionAttemptInProgress = false;
            lastReconnectAttempt = currentMillis;
        }
        return;
    }

    if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
        Serial.println("NetworkManager: Retrying WiFi connection...");
        connectWiFi();
    }
}