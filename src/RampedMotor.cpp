#include "RampedMotor.h"
#include "Config.h"

void RampedMotor::begin() {
    digitalWrite(pwmPin, LOW);
    pinMode(pwmPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    digitalWrite(dirPin, direction);
    stop();
}
void RampedMotor::run(uint8_t nextDirection, uint8_t speed) {
    requestedDirection = nextDirection;
    target = speed;
}
void RampedMotor::stop() {
    target = 0;
    pwm = 0;
    pausing = false;
    lastUpdate = millis();
    analogWrite(pwmPin, 0);
}
void RampedMotor::update(unsigned long now) {
    unsigned long elapsed = now - lastUpdate;
    lastUpdate = now;
    bool reversing = requestedDirection != direction;
    float desired = reversing ? 0 : target;
    float step = Config::rampMs == 0 ? 255.0f : 255.0f * elapsed / Config::rampMs;
    if (pwm < desired) pwm = (pwm + step > desired) ? desired : pwm + step;
    else if (pwm > desired) pwm = (pwm - step < desired) ? desired : pwm - step;
    analogWrite(pwmPin, static_cast<int>(pwm));
    if (reversing && pwm == 0) {
        if (!pausing) { pausing = true; zeroSince = now; }
        if (now - zeroSince >= Config::reversalPauseMs) {
            direction = requestedDirection;
            digitalWrite(dirPin, direction);
            pausing = false;
        }
    } else pausing = false;
}
