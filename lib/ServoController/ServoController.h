#pragma once
#include <ESP32Servo.h>

class ServoController {
public:
    ServoController();
    void begin();
    void setTopServoAngle(int angle);
    void setLeftServoAngle(int angle);
    void setRightServoAngle(int angle);

private:
    Servo topServo;
    Servo leftServo;
    Servo rightServo;
};