#include "Statemachine.h"
#include "Config.h"
#include "AxisMotor.h"
#include "brushMotor.h"
Axis axis = Axis::Idle;
unsigned long stateSince = 0;
bool fault = false;

// Stop immediately on switch closure; debounce release for 20 ms.
struct Limit {
    uint8_t pin;
    bool active = false;
    unsigned long lastLow = 0;
    explicit Limit(uint8_t p) : pin(p) {}
    void update(unsigned long now) {
        if (digitalRead(pin) == LOW) { active = true; lastLow = now; }
        else if (now - lastLow >= 20) active = false;
    }
};
Limit top(Config::topLimit), bottom(Config::bottomLimit);

void stopAxis() { axisMotor.stop(); axis = Axis::Idle; }
void stopAll() { brushMotor.stop(); stopAxis(); }
void trip(const char *reason) {
    stopAll();
    fault = true;
    Serial1.print("FAULT "); Serial1.println(reason);
}
void startLeg(Axis next, unsigned long now) {
    axisMotor.stop();
    stateSince = now;
    axis = next;
    axisMotor.run(next == Axis::Up);
}
void updateAxis(unsigned long now) {
    if (top.active && bottom.active) {
        if (!fault) trip("BOTH_LIMITS");
        return;
    }
    switch (axis) {
        case Axis::Up:
            if (top.active) {
                axisMotor.stop();
                axis = Axis::Wait; stateSince = now;
            } else if (now - stateSince >= Config::travelTimeoutMs) trip("UP_TIMEOUT");
            break;
        case Axis::Wait:
            if (now - stateSince >= Config::topWaitMs) {
                if (bottom.active) stopAxis();
                else startLeg(Axis::Down, now);
            }
            break;
        case Axis::Down:
            if (bottom.active) { stopAxis(); Serial1.println("EVENT CYCLE_DONE"); }
            else if (now - stateSince >= Config::travelTimeoutMs) trip("DOWN_TIMEOUT");
            break;
        case Axis::Idle: break;
    }
}

void beginStateMachine() {
    axisMotor.begin(); brushMotor.begin();
    pinMode(Config::topLimit, INPUT_PULLUP);
    pinMode(Config::bottomLimit, INPUT_PULLUP);
}
void updateStateMachine(unsigned long now) {
    top.update(now); bottom.update(now);
    updateAxis(now);
}

bool bothLimits() { return top.active && bottom.active; }
bool topActive() { return top.active; }
bool bottomActive() { return bottom.active; }
Axis axisState() { return axis; }
bool hasFault() { return fault; }
void clearFault() { fault = false; }
bool startCycle(unsigned long now) {
    if (axis != Axis::Idle || fault) return false;
    if (top.active) { axis = Axis::Wait; stateSince = now; }
    else startLeg(Axis::Up, now);
    return true;
}
