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

void SettingsManager::setStartHour(int newStartHour){
    if (startHour == newStartHour) return;

    startHour = newStartHour;

    prefs.begin("settings", false);
    prefs.putUInt("startHour", startHour);
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
    startHour = prefs.getUInt("startHour", 0);
    latitude = prefs.getDouble("latitude", 0.0);
    longitude = prefs.getDouble("longitude", 0.0);
    timeZoneString = prefs.getString("tzString", "");
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

int SettingsManager::getStartHour() const {
    return startHour;
}

double SettingsManager::getLatitude() const {
    return latitude;
}

double SettingsManager::getLongitude() const {
    return longitude;
}

const String& SettingsManager::getTimeZoneString() const {
    return timeZoneString;
}

bool SettingsManager::isFullConfigured() const {
    return !ssid.isEmpty() && !password.isEmpty() && !apiKey.isEmpty() && !(latitude == 0.0 && longitude == 0.0);
}

bool SettingsManager::isNetworkConfigured() const {
    return !ssid.isEmpty() && !password.isEmpty();
}