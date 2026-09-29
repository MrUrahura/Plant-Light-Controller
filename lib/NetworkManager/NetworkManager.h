#pragma once
#include "SettingsManager.h"

class NetworkManager {
public:
    NetworkManager(const SettingsManager& settings);
    void begin();
    void connectWiFi();
    bool isConnected() const;
    bool isConfigured() const;
    void handle();

private:
    const SettingsManager& settings;
    unsigned long lastReconnectAttempt = 0;
    bool connectionAttemptInProgress = false;
    const unsigned long reconnectInterval = 10000; // Retry every 10 seconds
    const unsigned long connectionTimeout = 30000; // Allow slow access points time to respond
};