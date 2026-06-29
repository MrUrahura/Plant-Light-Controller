#include <Arduino.h>
#include "Pins.h"
#include "Plant.h"
#include "LightSensor.h"
#include "ServoController.h"

// Components declarations
Plant currentPlant;
LightSensor sensor;
ServoController blinds;

// Variables declarations
float currentLux;
float currentPPFD;
float currentDLI;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  sensor.begin();
  blinds.begin();

  Serial.println("System initialized.");
}

void loop() {
  // put your main code here, to run repeatedly:
  currentLux = sensor.readLightLevel();
  currentPPFD = currentLux / 54.0; // Convert lux to PPFD (approximation)
  currentDLI += currentPPFD * 60.0 / 1000000.0;

  Serial.print("Lux: ");
  Serial.println(currentLux);
  
  Serial.print("PPFD: ");
  Serial.println(currentPPFD);

  Serial.print("DLI: ");
  Serial.println(currentDLI);

  delay(1000);
}