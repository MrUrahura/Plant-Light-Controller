#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Auto-Drawer Booting...");
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Running...");
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}