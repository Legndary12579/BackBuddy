#include <Wire.h>
#include <MPU9250_asukiaaa.h>

MPU9250_asukiaaa mySensor;

float baseline = 0;
bool calibrated = false;

float slouchThreshold = 15.0;
unsigned long slouchStartTime = 0;
bool currentlySlouching = false;
unsigned long requiredSlouchDuration = 10000; // 10 seconds

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.print("BOOT OK");
  Wire.begin(21, 22);
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  Serial.print("calling waitForConsent()");
  waitForConsent(); 
  calibrate();
}

float getTilt() {
  mySensor.accelUpdate();
  float aX = mySensor.accelX();
  float aY = mySensor.accelY();
  float aZ = mySensor.accelZ();
  return atan2(sqrt(aX*aX + aY*aY), aZ) * 180.0 / PI;
}

void waitForConsent() {
  Serial.println("BackBuddy is ready to calibrate.");
  Serial.println("Sit upright in your normal good posture, then type 'y' and press Enter to begin.");

  while (true) {
    if (Serial.available() > 0) {
      char input = Serial.read();
      if (input == 'y' || input == 'Y') {
        Serial.println("Starting calibration...");
        break;
      }
    }
  }
}

void calibrate() {
  Serial.println("Sit upright. Calibrating in 3 seconds...");
  delay(3000);

  float sum = 0;
  int samples = 30;
  for (int i = 0; i < samples; i++) {
    sum += getTilt();
    delay(50);
  }
  baseline = sum / samples;
  calibrated = true;
  Serial.print("Baseline tilt set: ");
  Serial.println(baseline);
}

void loop() {
  float tilt = getTilt();
  float deviation = abs(tilt - baseline);

  if (deviation > slouchThreshold) {
    if (!currentlySlouching) {
      currentlySlouching = true;
      slouchStartTime = millis();
    } else if (millis() - slouchStartTime >= requiredSlouchDuration) {
      Serial.println("SLOUCHING TOO LONG - VIBRATE!");
    }
  } else {
    currentlySlouching = false;
  }

  Serial.print("Tilt: "); Serial.print(tilt, 2);
  Serial.print("  Deviation: "); Serial.print(deviation, 2);
  Serial.print("  Slouching: "); Serial.println(currentlySlouching ? "YES" : "NO");

  delay(1000);
}


