#include <Arduino.h>
#include <string.h>

// Change these settings to match the final wiring and application.
namespace Config {
constexpr uint8_t motorPwm = 5, motorDir = 4;
constexpr uint8_t axisPwm = 6, axisDir = 7;
constexpr uint8_t topLimit = 8, bottomLimit = 9;
constexpr uint8_t forwardLevel = HIGH, upLevel = HIGH;
constexpr unsigned long baud = 115200;
constexpr unsigned long communicationTimeoutMs = 3000;
constexpr unsigned long travelTimeoutMs = 120000;
constexpr unsigned long topWaitMs = 2000;
}

enum class Axis { Idle, Up, Wait, Down };
Axis axis = Axis::Idle;
unsigned long stateSince = 0, lastCommand = 0;
bool connected = false, fault = false;
char line[48];
size_t length = 0;
bool overflow = false;

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

void stopAxis() { analogWrite(Config::axisPwm, 0); axis = Axis::Idle; }
void stopAll() { analogWrite(Config::motorPwm, 0); stopAxis(); }
void trip(const char *reason) {
  stopAll();
  fault = true;
  Serial1.print("FAULT "); Serial1.println(reason);
}
void startLeg(Axis next, unsigned long now) {
  analogWrite(Config::axisPwm, 0);
  digitalWrite(Config::axisDir, next == Axis::Up ? Config::upLevel : !Config::upLevel);
  stateSince = now;
  axis = next;
  analogWrite(Config::axisPwm, 255);
}
void updateAxis(unsigned long now) {
  if (top.active && bottom.active) {
    if (!fault) trip("BOTH_LIMITS");
    return;
  }
  switch (axis) {
    case Axis::Up:
      if (top.active) {
        analogWrite(Config::axisPwm, 0);
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
bool isCommand(const char *s) { return strcmp(line, s) == 0; }
void handleCommand(unsigned long now) {
  bool known = isCommand("PING") || isCommand("STATUS") || isCommand("STOP") ||
      isCommand("CLEAR") || isCommand("MOTOR OFF") || isCommand("MOTOR FORWARD") ||
      isCommand("MOTOR REVERSE") || isCommand("AXIS START") || isCommand("AXIS STOP");
  if (!known) { Serial1.println("ERR COMMAND"); return; }
  connected = true; lastCommand = now;
  if (isCommand("STOP")) stopAll();
  else if (isCommand("MOTOR OFF")) analogWrite(Config::motorPwm, 0);
  else if (isCommand("AXIS STOP")) stopAxis();
  else if (isCommand("CLEAR")) {
    stopAll();
    if (top.active && bottom.active) { Serial1.println("ERR BOTH_LIMITS"); return; }
    fault = false;
  } else if (isCommand("STATUS")) {
    const char *names[] = {"IDLE", "UP", "WAIT", "DOWN"};
    Serial1.print("STATUS AXIS="); Serial1.print(names[static_cast<unsigned>(axis)]);
    Serial1.print(" TOP="); Serial1.print(top.active);
    Serial1.print(" BOTTOM="); Serial1.print(bottom.active);
    Serial1.print(" FAULT="); Serial1.println(fault);
    return;
  } else if (!isCommand("PING")) {
    if (fault) { Serial1.println("ERR FAULT_CLEAR_REQUIRED"); return; }
    if (isCommand("AXIS START")) {
      if (axis != Axis::Idle) { Serial1.println("ERR AXIS_BUSY"); return; }
      if (top.active) { axis = Axis::Wait; stateSince = now; }
      else startLeg(Axis::Up, now);
    } else {
      analogWrite(Config::motorPwm, 0);
      digitalWrite(Config::motorDir, isCommand("MOTOR FORWARD") ? Config::forwardLevel : !Config::forwardLevel);
      analogWrite(Config::motorPwm, 255);
    }
  }
  Serial1.println("OK");
}
void setup() {
  digitalWrite(Config::motorPwm, LOW); digitalWrite(Config::axisPwm, LOW);
  pinMode(Config::motorPwm, OUTPUT); pinMode(Config::axisPwm, OUTPUT);
  pinMode(Config::motorDir, OUTPUT); pinMode(Config::axisDir, OUTPUT);
  stopAll();
  pinMode(Config::topLimit, INPUT_PULLUP);
  pinMode(Config::bottomLimit, INPUT_PULLUP);
  Serial1.begin(Config::baud, SERIAL_8N1);
}
void loop() {
  unsigned long now = millis();
  top.update(now); bottom.update(now);
  if (connected && now - lastCommand >= Config::communicationTimeoutMs) {
    connected = false; trip("COMM_TIMEOUT");
  }
  updateAxis(now);
  // Bound parsing work to keep checking switches during continuous input.
  for (uint8_t n = 0; n < 32 && Serial1.available(); ++n) {
    char c = static_cast<char>(Serial1.read());
    if (c == '\r' || c == '\n') {
      if (overflow) Serial1.println("ERR LINE_TOO_LONG");
      else if (length) { line[length] = '\0'; handleCommand(millis()); }
      length = 0; overflow = false;
    } else if (!overflow) {
      if (length < sizeof(line) - 1) line[length++] = c;
      else overflow = true;
    }
  }
}
