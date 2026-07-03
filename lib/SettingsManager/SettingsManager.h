#pragma once
#include <Arduino.h>
#include <Preferences.h>

class SettingsManager {
public:
    SettingsManager();
    void load();
    void setWiFi(const String& ssid, const String& password);
    void setLocation(double latitude, double longitude);
    void setApiKey(const String& apiKey);
    void setTimeZoneString(const String& tzString);
    const String& getSSID() const;
    const String& getPassword() const;
    const String& getApiKey() const;
    double getLatitude() const;
    double getLongitude() const;
    const String& getTimeZoneString() const;
    int getStartHour() const;
    bool isFullConfigured() const;
    bool isWiFiConfigured() const;

private:
    Preferences prefs;
    String ssid;
    String password;
    String apiKey;
    double latitude;
    double longitude;
    String timeZoneString;
    int startHour;
};