#include <Arduino.h>
#include "ServoController.h"
#include "Pins.h"

ServoController::ServoController() {
    
}

void ServoController::begin() {
    topServo.attach(TOP_SERVO_PIN);
    leftServo.attach(LEFT_SERVO_PIN);
    rightServo.attach(RIGHT_SERVO_PIN);
}