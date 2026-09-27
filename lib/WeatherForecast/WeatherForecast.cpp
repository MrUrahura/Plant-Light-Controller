#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <cmath>
#include "WeatherForecast.h"

// Define M_PI if not defined in order to use it in calculations
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WeatherForecast::WeatherForecast(const SettingsManager& settings, NetworkManager& network, const TimeManager& timeManager)
    : settings(settings), network(network), timeManager(timeManager) { }

bool WeatherForecast::update() {
     // Calculate remaining DLI for the current hour to the end of the photoperiod
    recentSuccess = false;
    calculateRemainingDLI();

    // A BLE connection can temporarily contend with the WiFi radio while the
    // TLS handshake is starting. Retry the complete request with a new client
    // rather than retrying a stream that has already been consumed.
    if (!recentSuccess) {
        delay(250);
        calculateRemainingDLI();
    }

    return recentSuccess;
}

double WeatherForecast::getCompleteDayDLI() const {
    return completeDayDLI;
}

double WeatherForecast::getPhotoperiodDLI() const {
    return photoperiodDLI;
}

double WeatherForecast::getRemainingDLI() const {
    return remainingDLI;
}

const std::array<double, 24>& WeatherForecast::getHourlyDLI() const {
    return hourlyDLI;
}

// Private helper functions
void WeatherForecast::calculateRemainingDLI() {    
    int startHour = timeManager.getPhotoperiodStartTime();
    int endHour = timeManager.getPhotoperiodEndTime();
    
    // Guard rails to prevent buffer overflows or index out of bounds errors
    if (startHour < 0) startHour = 0;
    if (endHour > 23) endHour = 23;
    if (startHour > endHour) {
        Serial.println("Error: Start hour must be less than or equal to end hour.");
        return;
    }
    
    if(!network.isConnected()) network.connectWiFi();

    // Fetch weather data from an API and update internal state
    if (network.isConnected()) {
        WiFiClientSecure client;
        client.setInsecure();
        client.setTimeout(15000);
        client.setHandshakeTimeout(15);
        
        HTTPClient http;

        Serial.print("API Key: ");
        Serial.println(settings.getAPIKey());
        String url = "https://api.weatherapi.com/v1/forecast.json?key=" + String(settings.getAPIKey()) + "&q=" + String(settings.getLatitude(), 6) + "," + String(settings.getLongitude(), 6) + "&days=1&aqi=no&alerts=no";
        Serial.print("Connecting to: ");
        Serial.println(url);
        
        Serial.print("Unix time: ");
        Serial.println(timeManager.getUnixTime());
        http.begin(client, url);
        http.setReuse(false);
        http.useHTTP10(true);
        http.setTimeout(10000);
        int httpCode = http.GET();
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);

        if (httpCode > 0) {
            Serial.print("Content-Length reported: ");
            Serial.println(http.getSize());
            
            StaticJsonDocument<1024> filter; // Keep it small and on the stack

            // Explicitly drill down or step-assign the nested structure
            JsonObject forecastObj = filter["forecast"].to<JsonObject>();
            JsonArray forecastdayArr = forecastObj["forecastday"].to<JsonArray>();
            JsonObject dayZeroObj = forecastdayArr.add<JsonObject>();
            JsonArray hourArr = dayZeroObj["hour"].to<JsonArray>();
            JsonObject hourFields = hourArr.add<JsonObject>();

            // Set the specific data keys you want to keep
            hourFields["cloud"] = true;
            hourFields["condition"]["text"] = true;
            
            DynamicJsonDocument doc(4096); // 24KB buffer for the filtered elements
            
            DeserializationError jsonError = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
            if (jsonError) {
                Serial.print("JSON Parsing failed: ");
                Serial.println(jsonError.c_str());
                http.end();
                return;
            }

            JsonArray hourlyArray = doc["forecast"]["forecastday"][0]["hour"];
            // Reset DLI values before processing new data
            completeDayDLI = 0.0;
            remainingDLI = 0.0;
            photoperiodDLI = 0.0;
            for (int h = 0; h < 24; h++) {
                // Get the weather data for the hour
                int cloudCover = hourlyArray[h]["cloud"] | 0;
                String conditionText = hourlyArray[h]["condition"]["text"] | "";

                Serial.print("Hour ");
                Serial.print(h);
                Serial.print(": Cloud ");
                Serial.print(cloudCover);
                Serial.print("% Condition ");
                Serial.println(conditionText);

                // Calculate Solar Geometry
                double clearSkyGHI = calculateClearSkyGHI(h);
                
                // Attenuate for clouds, considering the amount of cloud cover and then using a linear function within each zone
                double cloudMultiplier;
                if (cloudCover <= 25) {
                    cloudMultiplier = 1.0 - (1.0 - 0.89) * (cloudCover / 25.0);
                } else if (cloudCover <= 50) {
                    cloudMultiplier = 0.89 - (0.89 - 0.73) * ((cloudCover - 25.0) / 25.0);
                } else if (cloudCover <= 88) {
                    cloudMultiplier = 0.73 - (0.73 - 0.32) * ((cloudCover - 50.0) / 38.0);
                } 
                // Handle the 100% / high-density cloud cover variations dynamically
                else {
                    if (conditionText.indexOf("Heavy rain") != -1 || conditionText.indexOf("Thunderstorm") != -1) {
                        cloudMultiplier = 0.01;  // Extreme storm sky (99% light blocked)
                    } else if (conditionText.indexOf("Moderate rain") != -1 || conditionText.indexOf("Nimbostratus") != -1) {
                        cloudMultiplier = 0.05;  // Very dark/stormy overcast (95% light blocked)
                    } else if (conditionText.indexOf("Overcast") != -1 || conditionText.indexOf("Cloudy") != -1) {
                        cloudMultiplier = 0.15;  // Standard, thick low overcast (85% light blocked)
                    } else {
                        cloudMultiplier = 0.32;  // Thin/high overcast (Your default fallback)
                    }
                }
                double actualGHI = clearSkyGHI * cloudMultiplier;

                // Convert energy units to Photon Flux (PPFD)
                double ppfd = actualGHI * 0.45 * 4.6;

                // Accumulate into daily light integral moles
                double hourDLI = ppfd * 3600.0 / 1000000.0;
                completeDayDLI += hourDLI;
                hourlyDLI[h] = hourDLI;
                if (h >= startHour && h <= endHour) {
                    photoperiodDLI += hourDLI;
                    if(h >= timeManager.getHour()) {
                        remainingDLI += hourDLI;
                    }
                }
                recentSuccess = true;
            }
        } else {
            Serial.print("HTTP request failed. Code: ");
            Serial.println(httpCode);

            String response = http.getString();

            Serial.print("Response length: ");
            Serial.println(response.length());

            Serial.println("Response body:");
            Serial.println(response);
        }
        http.end();
    } else {
        Serial.println("Error: Not connected to WiFi. Maintaining previous DLI values.");
    }
    return;
}

// Function to calculate clear-sky global horizontal irradiance (GHI) in W/m²
// Uses basic solar geometry for a given latitude, day of year, and hour
double WeatherForecast::calculateClearSkyGHI(int hour) const {
    double latRad = settings.getLatitude() * M_PI / 180.0;
    
    // Declination angle of the sun
    double declination = 23.45 * sin(2.0 * M_PI * (284 + timeManager.getDayOfYear()) / 365.0) * M_PI / 180.0;
    
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