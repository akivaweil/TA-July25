#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <ESP32Servo.h>

class ServoControl {
private:
    Servo servo;
    int minPulseWidth;
    int maxPulseWidth;
    int minAngle;
    int maxAngle;

public:
    ServoControl();

    void init(int servoPin);
    void write(float angle);
    void writeMicroseconds(int microseconds);
    void detach();

    // Setters for customization
    void setPulseWidthRange(int minUs, int maxUs);
    void setAngleRange(int minDeg, int maxDeg);
};

#endif 