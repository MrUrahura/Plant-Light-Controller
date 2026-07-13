#pragma once
#include <Arduino.h>
#include <Preferences.h>

class SettingsManager {
public:
    SettingsManager();
    void load();
    void setWiFi(const String& newSsid, const String& newPassword);
    void setAPIKey(const String& apiKey);
    void setStartHour(int startHour);
    void setLocation(double newLat, double newLong);
    void setTimeZoneString(const String& tzString);
    const String& getSSID() const;
    const String& getPassword() const;
    const String& getAPIKey() const;
    double getLatitude() const;
    double getLongitude() const;
    const String& getTimeZoneString() const;
    int getStartHour() const;
    bool isFullConfigured() const;
    bool isNetworkConfigured() const;

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