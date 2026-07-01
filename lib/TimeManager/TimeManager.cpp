#include <time.h>

void TimeManager::begin() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time synchronization...");
    while (time(nullptr) < 100000) {
        delay(100);
    }
    Serial.println("Time synchronized.");
}