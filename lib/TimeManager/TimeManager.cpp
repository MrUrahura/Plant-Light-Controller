#include <time.h>
#include "TimeManager.h"

void TimeManager::TimeManager(const SettingsManager& settings, const NetworkManager& network, const int& photoperiod)
    : settings(settings), network(network), photoperiod(photoperiod) { }

void TimeManager::begin() {
    if(network.isConnected()) {
        // Point the core clock engine to standard global time servers
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("Waiting for time synchronization...");

        while (time(nullptr) < 100000) {
            delay(100);
        }

        // Pull the raw POSIX string out of your settings manager pointer (e.g., "PST8PDT,M3.2.0,M11.1.0")
        String tzRule = settings.getTimeZoneString();
        
        // Inject the rule directly into the ESP32 system environment variables
        setenv("TZ", tzRule.c_str(), 1); 
        // Forces the system clock to instantly re-calculate local time based on the new rule
        tzset();

        Serial.println("Time synchronized.");
    } else {
        Serial.println("Error: Not connected to WiFi. Time synchronization skipped.");
    }
}

void TimeManager::getCurrentHour(int &hour, int &minute, int &second) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    hour = timeinfo->tm_hour;
    minute = timeinfo->tm_min;
    second = timeinfo->tm_sec;
}

void TimeManager::getCurrentDate(int &year, int &month, int &day) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    year = timeinfo->tm_year + 1900;
    month = timeinfo->tm_mon + 1;
    day = timeinfo->tm_mday;
}

String TimeManager::getCurrentDateString() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[11]; // YYYY-MM-DD + null terminator
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return String(buffer);
}

int TimeManager::getHour() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_hour;
}

int TimeManager::getDayOfYear() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_yday + 1; // tm_yday is 0-based, so add 1
}

int TimeManager::getPhotoperiodEndTime() {
    return settings.getStartHour() + photoperiod;
}

bool TimeManager::withinPhotoperiod() {
    int currentHour = getHour();
    return (currentHour >= startHour && currentHour < startHour + photoperiod);
}