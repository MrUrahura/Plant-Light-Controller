#include <Arduino.h>
#include "ServoController.h"
#include "Pins.h"

ServoController::ServoController()
    : shadeState(static_cast<uint8_t>(ShadeID::ALL)),
    targetShadeState(static_cast<uint8_t>(ShadeID::ALL)),
    hasPendingState(false),
    topMoving(false),
    leftMoving(false),
    rightMoving(false)
{
}

void ServoController::begin() {

    // //Calibration Checks (Assuming all shades are closed at startup)
    // attachServos();
    // delay(1000);
    // //Top Check
    // topServo.write(REVERSE_SPEED);
    // delay(1000);
    // topServo.write(FORWARD_SPEED);
    // delay(900);
    // topServo.write(SERVO_STOP);
    // //Left Check
    // leftServo.write(REVERSE_SPEED);
    // delay(1000);
    // leftServo.write(FORWARD_SPEED);
    // delay(900);
    // leftServo.write(SERVO_STOP);
    // //Right Check
    // rightServo.write(FORWARD_SPEED);
    // delay(1000);
    // rightServo.write(REVERSE_SPEED);
    // delay(RIGHT_CLOSE_TIME_MS);
    // rightServo.write(SERVO_STOP);
    // delay(900);

    // delay(500);
    // topServo.write(SERVO_STOP);
    // leftServo.write(SERVO_STOP);
    // rightServo.write(SERVO_STOP);
    // delay(500);
    // detachServos();

    topMoving = false;
    leftMoving = false;
    rightMoving = false;

    // Regular Operation Code
    // prefs.begin("servos", true);
    // shadeState = prefs.getUChar("shadeState", static_cast<uint8_t>(ShadeID::ALL));
    // prefs.end();
    
    // Calibration Code: assume all shades begin closed due to manual intervention.
    shadeState = static_cast<uint8_t>(ShadeID::ALL);
    targetShadeState = shadeState;
    hasPendingState = false;
    saveOnComplete = false;
}

uint8_t ServoController::getCurrentState() {
    return shadeState;
}

bool ServoController::areShadesClosed(ShadeID shades) const {
    return (shadeState & shades) != 0;
}

bool ServoController::setShades(ShadeID shades, bool saveToPrefs) {
    return setShades(static_cast<uint8_t>(shades), saveToPrefs);
}

bool ServoController::setShades(uint8_t shades, bool saveToPrefs) {
    Serial.print("SERVO COMMAND: current=");
    Serial.print(shadeState);
    Serial.print(" target=");
    Serial.println(static_cast<uint8_t>(shades));
    
    // A new movement cannot interrupt an existing movement.
    if (isMoving()) {
        Serial.println("ServoController: Command rejected - servos are still moving.");
        return false;
    }

    // Already at requested state.
    if (shades == shadeState) {
        Serial.println("ServoController: Already at requested state.");
        return true;
    }

    attachServos();
    saveOnComplete = saveToPrefs;
    
    targetShadeState = shades;
    hasPendingState = true;

    // Top
    if ((shadeState & static_cast<uint8_t>(ShadeID::TOP_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::TOP_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::TOP_SHADE))
            moveServo(topServo, FORWARD_SPEED, TOP_CLOSE_TIME_MS);
        else
            moveServo(topServo, REVERSE_SPEED, TOP_OPEN_TIME_MS);
    }

    // Left
    if ((shadeState & static_cast<uint8_t>(ShadeID::LEFT_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::LEFT_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::LEFT_SHADE))
            moveServo(leftServo, FORWARD_SPEED, LEFT_CLOSE_TIME_MS);
        else
            moveServo(leftServo, REVERSE_SPEED, LEFT_OPEN_TIME_MS);
    }

    // Right
    if ((shadeState & static_cast<uint8_t>(ShadeID::RIGHT_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::RIGHT_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::RIGHT_SHADE))
            moveServo(rightServo, REVERSE_SPEED, RIGHT_CLOSE_TIME_MS);
        else
            moveServo(rightServo, FORWARD_SPEED, RIGHT_OPEN_TIME_MS);
    }

    return true;
}

void ServoController::moveServo(Servo& servo, uint8_t speed, uint32_t timeMs) {
    Serial.print("MOVING: ");
    if (&servo == &topServo){
        Serial.print("TOP");
    } else if (&servo == &leftServo) {
        Serial.print("LEFT");
    } else if (&servo == &rightServo) {
        Serial.print("RIGHT");
    }
    Serial.print(" speed=");
    Serial.print(speed);
    Serial.print(" duration=");
    Serial.println(timeMs);

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

    // Only now, after every servo has actually finished, does the controller commit the new state.
    if (hasPendingState && !isMoving())
    {
        shadeState = targetShadeState;
        hasPendingState = false;

        if (saveOnComplete) {
            digitalWrite(LED_BUILTIN, HIGH);
            saveState();
            Serial.println("ServoController: State saved to NVS.");
            saveOnComplete = false;
        } else {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("ServoController: Scan step complete (NVS save skipped).");
        }

        detachServos();
        Serial.print("ServoController: Movement complete. State = ");
        Serial.println(shadeState);
    }
}

void ServoController::saveState() {
    prefs.begin("servos", false);
    prefs.putUChar("shadeState", shadeState);
    prefs.end();
}

void ServoController::attachServos() {
    if (!topServo.attached()) {
        topServo.attach(TOP_SERVO_PIN);
    }
    if (!leftServo.attached()) {
        leftServo.attach(LEFT_SERVO_PIN);
    }
    if (!rightServo.attached()) {
        rightServo.attach(RIGHT_SERVO_PIN);
    }
}

void ServoController::detachServos() {
    if (topServo.attached()) {
        topServo.detach();
    }
    if (leftServo.attached()) {
        leftServo.detach();
    }
    if (rightServo.attached()) {
        rightServo.detach();
    }
}