#include <Preferences.h>
#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    // Do nothing for lightweight constructor. Actual initialization is done in load().
}

void SettingsManager::setWiFi(const String& newSsid, const String& newPassword) {
    if (ssid == newSsid && password == newPassword) return; // Prevent wear if unchanged

    ssid = newSsid;
    password = newPassword;

    prefs.begin("settings", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
}

void SettingsManager::setAPIKey(const String& newApiKey) {
    if (apiKey == newApiKey) return;

    apiKey = newApiKey;

    prefs.begin("settings", false);
    prefs.putString("apiKey", apiKey);
    prefs.end();
}

void SettingsManager::setLocation(double newLat, double newLng) {
    if (latitude == newLat && longitude == newLng) return;

    latitude = newLat;
    longitude = newLng;

    prefs.begin("settings", false);
    prefs.putDouble("latitude", latitude);
    prefs.putDouble("longitude", longitude);
    prefs.end();
}

void SettingsManager::setTimeZoneString(const String& tzString) {
    if (timeZoneString == tzString) return;

    timeZoneString = tzString;

    prefs.begin("settings", false);
    prefs.putString("tzString", timeZoneString);
    prefs.end();
}

void SettingsManager::load() {
    prefs.begin("settings", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    apiKey = prefs.getString("apiKey", "");
    latitude = prefs.getDouble("latitude", 0.0);
    longitude = prefs.getDouble("longitude", 0.0);
    timeZoneString = prefs.getString("timeZoneString", "");
    prefs.end();
}

const String& SettingsManager::getSSID() const {
    return ssid;
}

const String& SettingsManager::getPassword() const {
    return password;
}

const String& SettingsManager::getAPIKey() const {
    return apiKey;
}

double SettingsManager::getLatitude() const {
    return latitude;
}

const String& SettingsManager::getTimeZoneString() const {
    return timeZoneString;
}

int SettingsManager::getStartHour() const {
    return startHour;
}

double SettingsManager::getLongitude() const {
    return longitude;
}

bool SettingsManager::isFullConfigured() const {
    return !ssid.isEmpty() && !password.isEmpty() && !apiKey.isEmpty() && !(latitude == 0.0 && longitude == 0.0);
}

bool SettingsManager::isNetworkConfigured() const {
    return !ssid.isEmpty() && !password.isEmpty();
}