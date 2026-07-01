#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // Install "ArduinoJson" via the Arduino Library Manager
#include <cmath>

// Define M_PI if not defined in order to use it in calculations
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Network and API Configurations
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_WEATHERAPI_KEY";

// Location & Date parameters
const double latitude = 0; // Set your latitude for solar geometry calculations
const int dayOfYear   = 0; // Calculate the day of the year based on the date

// DLI information
double totalDLI = 0.0;
double remainingDLI = 0.0;

void begin() {
    // Initialize any necessary components here
    // Get SSID, password, and API key from secure storage or configuration
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    totalDLI = getRemainingDLI(0, 23); // Calculate remaining DLI for the whole day

}

void update(int startHour, int endHour) {
    // Fetch weather data from an API and update internal state
    calculateRemainingDLI(startHour, endHour);
}

double getPredictedRemainingDLI() {
    return remainingDLI;
}

private void calculateRemainingDLI(int startHour, int endHour) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }
    
    // Guard rails to prevent buffer overflows or index out of bounds errors
    if (startHour < 0) startHour = 0;
    if (endHour > 23) endHour = 23;
    if (startHour > endHour) {
        Serial.println("Error: Start hour must be less than or equal to end hour.");
        return;
    }
    
    // Fetch weather data from an API and update internal state
    totalDLI = 0.0;
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://weatherapi.com" + String(apiKey) + "&q=" + String(location) + "&dt=" + String(date));
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
        
            // DynamicJsonDocument sizing: WeatherAPI history responses are quite large.
            // We filter the JSON input to ONLY read the hourly cloud cover data to save ESP32 memory.
            DynamicJsonDocument filter(2048);
            filter["forecast"]["forecastday"][0]["hour"][0]["cloud"] = true;

            DynamicJsonDocument doc(16384); // Allocate enough heap space for the filtered layout
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

            if (error) {
                Serial.print("JSON Parsing failed: ");
                Serial.println(error.c_str());
                return;
            }

            JsonArray hourlyArray = doc["forecast"]["forecastday"][0]["hour"];
            totalDLI = 0.0;
            remainingDLI = 0.0;
            for (int h = 0; h <= 23; h++) {
                int cloudCover = hourlyArray[h]["cloud"].as<int>();

                // Calculate Solar Geometry
                double clearSkyGHI = calculateClearSkyGHI(latitude, dayOfYear, h);
                
                // Attenuate for clouds
                double cloudMultiplier = 1.0 - (0.75 * (cloudCover / 100.0));
                double actualGHI = clearSkyGHI * cloudMultiplier;

                // Convert energy units to Photon Flux (PPFD)
                double ppfd = actualGHI * 0.45 * 4.6;

                // Accumulate into daily light integral moles
                double hourlyDLI = ppfd * 3600.0 / 1000000.0;
                totalDLI += hourlyDLI;
                if (h >= startHour && h <= endHour) {
                    remainingDLI += hourlyDLI;
                }
            }
            Serial.print("Total DLI for ");
            Serial.print(date);
            Serial.print(": ");
            Serial.println(totalDLI);
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
    return;
}

// Function to calculate clear-sky global horizontal irradiance (GHI) in W/m²
// Uses basic solar geometry for a given latitude, day of year, and hour
private double calculateClearSkyGHI(double latitude, int dayOfYear, int hour) {
    double latRad = latitude * M_PI / 180.0;
    
    // Declination angle of the sun
    double declination = 23.45 * sin(2.0 * M_PI * (284 + dayOfYear) / 365.0) * M_PI / 180.0;
    
    // Hour angle (Solar noon is 12, each hour is 15 degrees)
    double hourAngle = (hour - 12) * 15.0 * M_PI / 180.0;
    
    // Solar altitude angle (angle above the horizon)
    double sinSolarAltitude = sin(latRad) * sin(declination) + cos(latRad) * cos(declination) * cos(hourAngle);
    
    if (sinSolarAltitude <= 0) return 0.0; // Nighttime

    // Simple clear-sky solar constant model adjusted for atmospheric attenuation
    const double solarConstant = 1367.0; 
    double transmissionCoeff = 0.7; // Typical clear sky transmission
    
    return solarConstant * transmissionCoeff * sinSolarAltitude;
}