#include <Arduino.h>
#include "ServoController.h"
#include "Pins.h"

ServoController::ServoController() {
    
}

void ServoController::begin() {
    shadeState = static_cast<uint8_t>(ShadeID::NONE);
}

void ServoController::begin() {
    topServo.attach(TOP_SERVO_PIN);
    leftServo.attach(LEFT_SERVO_PIN);
    rightServo.attach(RIGHT_SERVO_PIN);
}

uint8_t ServoController::getCurrentState() {
    return shadeState;
}

bool ServoController::areShadesClosed(ShadeID shades) const {
    return (shadeState & shades) != 0;
}

void ServoController::setShades(ShadeID shades) {
    setShades(static_cast<uint8_t>(shades));
}

void ServoController::setShades(uint8_t shades) {
    shadeState |= shades;
    
    if (shadeState & ShadeID::TOP_SHADE) {
        // close the shade
    } else {
        // open the shade
    }

    if (shadeState & ShadeID::LEFT_SHADE) {
        // close the shade
    } else {
        // open the shade
    }

    if (shadeState & ShadeID::RIGHT_SHADE) {
        // close the shade
    } else {
        // open the shade
    }
}