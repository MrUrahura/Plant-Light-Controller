#include <Arduino.h>
#include <WiFi.h>
#include "NetworkManager.h"
#include "SettingsManager.h"

NetworkManager::NetworkManager(const SettingsManager& settings)
    : settings(settings) {  }

void NetworkManager::begin(){
    WiFi.mode(WIFI_STA); 
    connectWiFi();
}

bool NetworkManager::isConfigured() const {
    return settings.isWiFiConfigured();
}

void NetworkManager::connectWiFi() {
    if (!isConfigured()) {
        Serial.println("NetworkManager: WiFi credentials or API key not configured.");
        return;
    }

    if (isConnected()) {
        return;
    }

    Serial.printf("NetworkManager: Initiating connection to %s...\n", settings.getSSID().c_str());

    WiFi.disconnect(false, true); 
    WiFi.begin(settings.getSSID().c_str(), settings.getPassword().c_str());

    lastReconnectAttempt = millis();
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::handle() {
    // Non-blocking reconnection logic
    if (!isConnected() && isConfigured()) {
        unsigned long currentMillis = millis();
        
        if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
            Serial.println("NetworkManager: Connection lost. Retrying...");
            connectWiFi(); 
        }
    }
}