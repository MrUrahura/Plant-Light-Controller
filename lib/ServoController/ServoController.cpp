#include <Arduino.h>
#include "ServoController.h"
#include "Pins.h"

ServoController::ServoController() {
    
}

void ServoController::begin() {
    // Attach Servos
    topServo.attach(TOP_SERVO_PIN);
    leftServo.attach(LEFT_SERVO_PIN);
    rightServo.attach(RIGHT_SERVO_PIN);
    
    prefs.begin("servos", true);
    setShades(prefs.getUChar("shadeState", static_cast<uint8_t>(ShadeID::ALL)));
    prefs.end();
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
    if(shades == shadeState) return;
    
    // Top
    if ((shadeState & ShadeID::TOP_SHADE) != (shades & ShadeID::TOP_SHADE))
    {
        if (shades & ShadeID::TOP_SHADE)
            moveServo(topServo, FORWARD_SPEED, TOP_CLOSE_TIME_MS);
        else
            moveServo(topServo, REVERSE_SPEED, TOP_OPEN_TIME_MS);
    }

    // Left
    if ((shadeState & ShadeID::LEFT_SHADE) != (shades & ShadeID::LEFT_SHADE))
    {
        if (shades & ShadeID::LEFT_SHADE)
            moveServo(leftServo, FORWARD_SPEED, LEFT_CLOSE_TIME_MS);
        else
            moveServo(leftServo, REVERSE_SPEED, LEFT_OPEN_TIME_MS);
    }

    // Right
    if ((shadeState & ShadeID::RIGHT_SHADE) != (shades & ShadeID::RIGHT_SHADE))
    {
        if (shades & ShadeID::RIGHT_SHADE)
            moveServo(rightServo, FORWARD_SPEED, RIGHT_CLOSE_TIME_MS);
        else
            moveServo(rightServo, REVERSE_SPEED, RIGHT_OPEN_TIME_MS);
    }

    moveLoop();

    shadeState = shades;
    saveState();
}

void ServoController::moveServo(Servo& servo, uint8_t speed, uint32_t timeMs)
{
    servo.write(speed);
    topMoving = true;
    topStopTime = millis() + (speed == FORWARD_SPEED ? TOP_CLOSE_TIME_MS : TOP_OPEN_TIME_MS);
}

bool ServoController::isMoving() const {
    return topMoving || leftMoving || rightMoving;
}

void ServoController::moveLoop() {
    while(isMoving()){
        uint32_t now = millis();

        if (topMoving && now >= topStopTime)
        {
            topServo.write(SERVO_STOP);
            topMoving = false;
        }

        if (leftMoving && now >= leftStopTime)
        {
            leftServo.write(SERVO_STOP);
            leftMoving = false;
        }

        if (rightMoving && now >= rightStopTime)
        {
            rightServo.write(SERVO_STOP);
            rightMoving = false;
        }
    }
}

void ServoController::saveState() {
    prefs.begin("servos", false);
    prefs.putUChar("shadeState", shadeState);
    prefs.end();
}