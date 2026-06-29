#include "ServoController.h"
#include "Pins.h"
#include <Wire.h>
#include <ESP32Servo.h>

Servo topServo;
Servo leftServo;
Servo rightServo;

void ServoController::begin() {
    leftServo.attach(LEFT_SERVO_PIN);
    rightServo.attach(RIGHT_SERVO_PIN);
}