#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "design.h"

// ===================== WIFI =====================
const char* ssid     = "ESP32-Robot";
const char* password = "12345678";

// ===================== PINS =====================
#define SERVO_LEFT_PIN   13
#define SERVO_RIGHT_PIN  14

#define LEFT_RPWM   16
#define LEFT_LPWM   17
#define RIGHT_RPWM  18
#define RIGHT_LPWM  19

// Relays (GPIO 35 is INPUT ONLY → changed to 27)
#define RELAY1_PIN  25
#define RELAY2_PIN  33
#define RELAY3_PIN  32
#define RELAY4_PIN  27      // ← Fixed (was 35)

#define RELAY_ON    LOW     // Active LOW
#define RELAY_OFF   HIGH

// ===================== SETTINGS =====================
#define MAX_ANGLE     310
#define SERVO_MIN_US  500
#define SERVO_MAX_US  2500
#define SMOOTH_STEP   2
#define SMOOTH_DELAY  8
#define RECORD_MAX    200
#define MOTOR_SPEED   180

Servo servoLeft;
Servo servoRight;
WebServer server(80);

int currentAngle = 0;
int recordBuffer[RECORD_MAX];
int recordCount = 0;
bool isRecording = false;

bool relayState[4] = {false, false, false, false};
const int relayPins[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// ===================== SERVO =====================
void moveServosSmooth(int target) {
  target = constrain(target, 0, MAX_ANGLE);
  while (currentAngle != target) {
    if (currentAngle < target) currentAngle += SMOOTH_STEP;
    else currentAngle -= SMOOTH_STEP;
    if (abs(currentAngle - target) < SMOOTH_STEP) currentAngle = target;

    int leftPulse  = map(currentAngle, 0, MAX_ANGLE, SERVO_MIN_US, SERVO_MAX_US);
    int rightPulse = map(currentAngle, 0, MAX_ANGLE, SERVO_MAX_US, SERVO_MIN_US);

    servoLeft.writeMicroseconds(leftPulse);
    servoRight.writeMicroseconds(rightPulse);

    if (isRecording && recordCount < RECORD_MAX) {
      recordBuffer[recordCount++] = currentAngle;
    }
    delay(SMOOTH_DELAY);
  }
}

void setServosInstant(int angle) {
  angle = constrain(angle, 0, MAX_ANGLE);
  currentAngle = angle;
  int leftPulse  = map(angle, 0, MAX_ANGLE, SERVO_MIN_US, SERVO_MAX_US);
  int rightPulse = map(angle, 0, MAX_ANGLE, SERVO_MAX_US, SERVO_MIN_US);
  servoLeft.writeMicroseconds(leftPulse);
  servoRight.writeMicroseconds(rightPulse);
}

// ===================== MOTOR =====================
void motorStop() {
  analogWrite(LEFT_RPWM, 0);  analogWrite(LEFT_LPWM, 0);
  analogWrite(RIGHT_RPWM, 0); analogWrite(RIGHT_LPWM, 0);
}
void motorForward() {
  analogWrite(LEFT_RPWM, MOTOR_SPEED);  analogWrite(LEFT_LPWM, 0);
  analogWrite(RIGHT_RPWM, MOTOR_SPEED); analogWrite(RIGHT_LPWM, 0);
}
void motorBackward() {
  analogWrite(LEFT_RPWM, 0);  analogWrite(LEFT_LPWM, MOTOR_SPEED);
  analogWrite(RIGHT_RPWM, 0); analogWrite(RIGHT_LPWM, MOTOR_SPEED);
}
void motorLeft() {
  analogWrite(LEFT_RPWM, 0);  analogWrite(LEFT_LPWM, MOTOR_SPEED);
  analogWrite(RIGHT_RPWM, MOTOR_SPEED); analogWrite(RIGHT_LPWM, 0);
}
void motorRight() {
  analogWrite(LEFT_RPWM, MOTOR_SPEED); analogWrite(LEFT_LPWM, 0);
  analogWrite(RIGHT_RPWM, 0); analogWrite(RIGHT_LPWM, MOTOR_SPEED);
}

// ===================== RELAY =====================
void setRelay(int num, bool state) {
  if (num < 1 || num > 4) return;
  relayState[num-1] = state;
  digitalWrite(relayPins[num-1], state ? RELAY_ON : RELAY_OFF);
}

// ===================== WEB =====================
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleServo() {
  if (server.hasArg("a")) {
    moveServosSmooth(server.arg("a").toInt());
  }
  server.send(200, "text/plain", "OK");
}

void handleMove() {
  if (server.hasArg("d")) {
    String d = server.arg("d");
    if (d == "F") motorForward();
    else if (d == "B") motorBackward();
    else if (d == "L") motorLeft();
    else if (d == "R") motorRight();
    else motorStop();
  }
  server.send(200, "text/plain", "OK");
}

void handleRelay() {
  if (server.hasArg("n")) {
    int num = server.arg("n").toInt();
    if (num >= 1 && num <= 4) {
      setRelay(num, !relayState[num-1]);
      server.send(200, "text/plain", relayState[num-1] ? "ON" : "OFF");
      return;
    }
  }
  server.send(200, "text/plain", "ERROR");
}

void handleRecord() {
  recordCount = 0;
  isRecording = true;
  server.send(200, "text/plain", "OK");
}

void handlePlay() {
  isRecording = false;
  if (recordCount == 0) {
    server.send(200, "text/plain", "Nothing recorded");
    return;
  }
  server.send(200, "text/plain", "Playing " + String(recordCount) + " steps");
  for (int i = 0; i < recordCount; i++) {
    setServosInstant(recordBuffer[i]);
    delay(SMOOTH_DELAY);
  }
}

void handleHome() {
  isRecording = false;
  moveServosSmooth(0);
  motorStop();
  server.send(200, "text/plain", "Home");
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servoLeft.setPeriodHertz(50);
  servoRight.setPeriodHertz(50);
  servoLeft.attach(SERVO_LEFT_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servoRight.attach(SERVO_RIGHT_PIN, SERVO_MIN_US, SERVO_MAX_US);

  pinMode(LEFT_RPWM, OUTPUT);
  pinMode(LEFT_LPWM, OUTPUT);
  pinMode(RIGHT_RPWM, OUTPUT);
  pinMode(RIGHT_LPWM, OUTPUT);
  motorStop();

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_OFF);
  }

  setServosInstant(0);

  WiFi.softAP(ssid, password);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/move", handleMove);
  server.on("/relay", handleRelay);
  server.on("/record", handleRecord);
  server.on("/play", handlePlay);
  server.on("/home", handleHome);
  server.begin();

  Serial.println("Ready!");
}

void loop() {
  server.handleClient();
}
