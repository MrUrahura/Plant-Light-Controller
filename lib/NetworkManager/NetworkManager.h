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
    const unsigned long reconnectInterval = 30000; // Retry every 30 seconds
};