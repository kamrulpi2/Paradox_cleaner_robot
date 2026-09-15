#ifndef AUTO_H
#define AUTO_H

#include <Wire.h>
#include "HUSKYLENS.h"

// =====================================================================
//  AUTONOMOUS MODE  (Gravity HuskyLens AI Camera - Object Tracking)
//  ---------------------------------------------------------------
//  Setup step you must do once: on the HuskyLens screen pick
//  "Object Tracking", point it at the bottle, press the LEARN button.
//  After that this code follows the learned bottle automatically.
// =====================================================================

// ---- Functions defined in main.ino, used here to drive the robot ----
extern void motorForward();
extern void motorLeft();
extern void motorRight();
extern void motorStop();
extern void moveServosSmooth(int target);

HUSKYLENS huskylens;

// ---------- TUNABLE PARAMETERS ----------
const int HL_SDA_PIN       = 21;   // ESP32 default I2C pins
const int HL_SCL_PIN       = 22;
const int SCREEN_CENTER_X  = 160;  // HuskyLens screen is 320 x 240
const int DEADZONE_PX      = 30;   // left/right decision margin around center
const int GRAB_WIDTH_PX    = 120;  // bounding-box width that means "bottle ~8cm away"
                                    // -> watch Serial Monitor "camW" and tune this number
const int GRAB_ANGLE       = 155;  // claw angle used to catch the bottle (same 0-310 scale
                                    // as the manual servo slider)
const unsigned long GRAB_HOLD_MS = 800;

// ---------- AUTONOMOUS STATE (read by main.ino for the /status page) ----------
String autoStatus    = "SEARCHING";  // SEARCHING / APPROACHING / GRABBING / NO CAMERA
int    bottlesCaught = 0;
int    lastCamX      = -1;
int    lastCamW      = -1;
bool   isGrabbing     = false;
bool   huskyLensOk    = false;

// ---------- INIT (call once from setup()) ----------
void initHuskyLens() {
  Wire.begin(HL_SDA_PIN, HL_SCL_PIN);

  for (int i = 0; i < 5; i++) {
    if (huskylens.begin(Wire)) {
      huskyLensOk = true;
      break;
    }
    Serial.println("HuskyLens not found - retrying...");
    delay(300);
  }

  if (huskyLensOk) {
    huskylens.writeAlgorithm(ALGORITHM_OBJECT_TRACKING);
    Serial.println("HuskyLens ready (Object Tracking mode)");
  } else {
    Serial.println("HuskyLens NOT connected - AUTO mode will stay idle until it is wired up");
  }
}

// ---------- GRAB SEQUENCE ----------
void grabAndDrop() {
  isGrabbing = true;
  autoStatus = "GRABBING";
  motorStop();

  moveServosSmooth(GRAB_ANGLE);   // claw closes over the bottle
  delay(GRAB_HOLD_MS);
  moveServosSmooth(0);            // claw opens, bottle drops into the storage box

  bottlesCaught++;
  isGrabbing = false;
  autoStatus = "SEARCHING";
}

// ---------- MAIN AUTONOMOUS STEP - call every loop() while mode == AUTO ----------
void autoModeLoop() {
  if (!huskyLensOk) {
    autoStatus = "NO CAMERA";
    motorStop();
    return;
  }

  if (isGrabbing) return;   // busy grabbing, skip this cycle

  if (!huskylens.request() || !huskylens.isLearned() || !huskylens.available()) {
    motorStop();
    autoStatus = "SEARCHING";
    lastCamX = -1;
    lastCamW = -1;
    return;
  }

  HUSKYLENSResult result = huskylens.read();
  lastCamX = result.xCenter;
  lastCamW = result.width;

  if (lastCamW >= GRAB_WIDTH_PX) {
    grabAndDrop();
    return;
  }

  autoStatus = "APPROACHING";

  if (lastCamX < SCREEN_CENTER_X - DEADZONE_PX) {
    motorLeft();
  } else if (lastCamX > SCREEN_CENTER_X + DEADZONE_PX) {
    motorRight();
  } else {
    motorForward();
  }
}

#endif
