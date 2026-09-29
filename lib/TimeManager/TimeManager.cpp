#include <time.h>
#include "TimeManager.h"

TimeManager::TimeManager(const SettingsManager& settings, const NetworkManager& network, const Plant& plant)
    : settings(settings), network(network), plant(plant) { }

void TimeManager::begin() {
    if (prefs.begin("timeManager", false)) {
        uint8_t storedInterval = prefs.getUChar("optInterval", 30);
        if (storedInterval == 5 || storedInterval == 10 || storedInterval == 15 ||
            storedInterval == 20 || storedInterval == 30 || storedInterval == 60) {
            optimizationIntervalMinutes = storedInterval;
        }
        lastOptimizationTime = static_cast<time_t>(prefs.getULong64("lastOptimize", 0));
        prefs.end();
    } else {
        Serial.println("TimeManager: Warning: Could not open preferences; using defaults.");
    }

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
            Serial.println(settings.getTimeZoneString());
            
            // Inject the rule directly into the ESP32 system environment variables
            setenv("TZ", tzRule.c_str(), 1); 
            // Forces the system clock to instantly re-calculate local time based on the new rule
            tzset();

            Serial.println("TimeManager: Time synchronized.");
            Serial.print("Current hour: ");
            Serial.println(getHour());
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
    return settings.getStartHour() + plant.getPhotoperiod();
}

bool TimeManager::withinPhotoperiod() const {
    int currentHour = getHour();
    int startHour = settings.getStartHour();
    return (currentHour >= startHour && currentHour < startHour + plant.getPhotoperiod());
}

time_t TimeManager::getUnixTime() const {
    return time(nullptr);
}

bool TimeManager::setOptimizationIntervalMinutes(int intervalMinutes) {
    if (intervalMinutes != 5 && intervalMinutes != 10 && intervalMinutes != 15 &&
        intervalMinutes != 20 && intervalMinutes != 30 && intervalMinutes != 60) {
        Serial.println("TimeManager: Rejected unsupported optimization interval.");
        return false;
    }

    if (optimizationIntervalMinutes == intervalMinutes) {
        return true;
    }

    optimizationIntervalMinutes = static_cast<uint8_t>(intervalMinutes);
    prefs.begin("timeManager", false);
    prefs.putUChar("optInterval", optimizationIntervalMinutes);
    prefs.end();
    return true;
}

uint8_t TimeManager::getOptimizationIntervalMinutes() const {
    return optimizationIntervalMinutes;
}

time_t TimeManager::getNextOptimizationTime() const {
    if (!isTimeSynced()) {
        return 0;
    }

    if (lastOptimizationTime == 0) {
        return getUnixTime();
    }

    struct tm nextLocalTime = *localtime(&lastOptimizationTime);
    nextLocalTime.tm_min =
        (nextLocalTime.tm_min / optimizationIntervalMinutes + 1) * optimizationIntervalMinutes;
    nextLocalTime.tm_sec = 0;
    nextLocalTime.tm_isdst = -1;

    time_t nextOptimization = mktime(&nextLocalTime);
    while (nextOptimization <= lastOptimizationTime) {
        nextLocalTime.tm_min += optimizationIntervalMinutes;
        nextLocalTime.tm_isdst = -1;
        nextOptimization = mktime(&nextLocalTime);
    }
    return nextOptimization;
}

int32_t TimeManager::getSecondsUntilNextOptimization() const {
    const time_t nextOptimization = getNextOptimizationTime();
    if (nextOptimization == 0) {
        return -1;
    }
    const time_t remaining = nextOptimization - getUnixTime();
    return remaining > 0 ? static_cast<int32_t>(remaining) : 0;
}

int32_t TimeManager::getSecondsSincePreviousOptimization() const {
    if (lastOptimizationTime == 0 || !isTimeSynced()) {
        return -1;
    }
    const time_t elapsed = getUnixTime() - lastOptimizationTime;
    return elapsed > 0 ? static_cast<int32_t>(elapsed) : 0;
}

void TimeManager::recordOptimization() {
    if (!isTimeSynced()) {
        Serial.println("TimeManager: Cannot record an optimization without synchronized time.");
        return;
    }
    lastOptimizationTime = getUnixTime();
    prefs.begin("timeManager", false);
    prefs.putULong64("lastOptimize", static_cast<uint64_t>(lastOptimizationTime));
    prefs.end();
}