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

    topServo.write(SERVO_STOP);
    leftServo.write(SERVO_STOP);
    rightServo.write(SERVO_STOP);
    topMoving = false;
    leftMoving = false;
    rightMoving = false;
    
    /* Regular Operation Code
        prefs.begin("servos", true);
        shadeState = prefs.getUChar("shadeState", static_cast<uint8_t>(ShadeID::ALL));
        prefs.end();
    */
    // Calibration Code
    shadeState = static_cast<uint8_t>(ShadeID::ALL);
    saveState();
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
            moveServo(rightServo, REVERSE_SPEED, RIGHT_CLOSE_TIME_MS);
        else
            moveServo(rightServo, FORWARD_SPEED, RIGHT_OPEN_TIME_MS);
    }

    shadeState = shades;
    saveState();
}

void ServoController::moveServo(Servo& servo, uint8_t speed, uint32_t timeMs)
{
    Serial.println("Moving servo...");
    servo.write(speed);
    if (&servo == &topServo) {
        topMoving = true;
        topStartTime = millis();
        topDuration = timeMs;
    }
    else if (&servo == &leftServo) {
        leftMoving = true;
        leftStartTime = millis();
        leftDuration = timeMs;
    }
    else if (&servo == &rightServo) {
        rightMoving = true;
        rightStartTime = millis();
        rightDuration = timeMs;
    }
}

bool ServoController::isMoving() const {
    return topMoving || leftMoving || rightMoving;
}

void ServoController::update() {
    uint32_t now = millis();

    if (topMoving && (now - topStartTime >= topDuration))
    {
        topServo.write(SERVO_STOP);
        topMoving = false;
        Serial.println("Stopping top servo...");
    }

    if (leftMoving && (now - leftStartTime >= leftDuration))
    {
        leftServo.write(SERVO_STOP);
        leftMoving = false;
        Serial.println("Stopping left servo...");
    }

    if (rightMoving && (now - rightStartTime >= rightDuration))
    {
        rightServo.write(SERVO_STOP);
        rightMoving = false;
        Serial.println("Stopping right servo...");
    }
}

void ServoController::saveState() {
    prefs.begin("servos", false);
    prefs.putUChar("shadeState", shadeState);
    prefs.end();
}