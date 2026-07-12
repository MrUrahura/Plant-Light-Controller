#include <time.h>
#include "TimeManager.h"

TimeManager::TimeManager(const SettingsManager& settings, const NetworkManager& network, const int& photoperiod)
    : settings(settings), network(network), photoperiod(photoperiod) { }

void TimeManager::begin() {
    if(network.isConnected()) {
        // Point the core clock engine to standard global time servers
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("Waiting for time synchronization...");

        unsigned long startAttempt = millis();
        while (!isTimeSynced() && (millis() - startAttempt < 5000)) {
            delay(100);
        }

        if(isTimeSynced()){
            // Pull the raw POSIX string out of your settings manager pointer (e.g., "PST8PDT,M3.2.0,M11.1.0")
            String tzRule = settings.getTimeZoneString();
            
            // Inject the rule directly into the ESP32 system environment variables
            setenv("TZ", tzRule.c_str(), 1); 
            // Forces the system clock to instantly re-calculate local time based on the new rule
            tzset();

            Serial.println("TimeManager: Time synchronized.");
        } else {
            Serial.println("TimeManager: Warning: NTP response timed out. Will retry in background loop.");
        }
    } else {
        Serial.println("TimeManager: Warning: Not connected to WiFi. Time synchronization skipped. Will retry in background loop.");
    }
}

bool TimeManager::isTimeSynced() const {
    // 1767225600 is Jan 1, 2026. If system time is larger, NTP sync was successful.
    return (time(nullptr) > 1767225600);
}

void TimeManager::getCurrentTime(int &hour, int &minute, int &second) const {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    hour = timeinfo->tm_hour;
    minute = timeinfo->tm_min;
    second = timeinfo->tm_sec;
}

void TimeManager::getCurrentDate(int &year, int &month, int &day) const {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    year = timeinfo->tm_year + 1900;
    month = timeinfo->tm_mon + 1;
    day = timeinfo->tm_mday;
}

String TimeManager::getCurrentDateString() const {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[11]; // YYYY-MM-DD + null terminator
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return String(buffer);
}

int TimeManager::getHour() const {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_hour;
}

int TimeManager::getDayOfYear() const {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_yday + 1; // tm_yday is 0-based, so add 1
}

int TimeManager::getPhotoperiodStartTime() const {
    return settings.getStartHour();
}

int TimeManager::getPhotoperiodEndTime() const {
    return settings.getStartHour() + photoperiod;
}

bool TimeManager::withinPhotoperiod() const {
    int currentHour = getHour();
    int startHour = settings.getStartHour();
    return (currentHour >= startHour && currentHour < startHour + photoperiod);
}

time_t TimeManager::getUnixTime() const {
    return time(nullptr);
}