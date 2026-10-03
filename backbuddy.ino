#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define MOTOR_PIN 13
MPU9250_asukiaaa mySensor;

float baseline = 0;
bool calibrated = false;

float slouchThreshold = 15.0;
unsigned long slouchStartTime = 0;
bool currentlySlouching = false;
unsigned long requiredSlouchDuration = 10000; // 10 seconds
float gyroThreshold = 15.0;        // above this = possible movement
unsigned long movementStartTime = 0;
bool sustainedMovement = false;
unsigned long requiredMovementDuration = 1000; // 1 second of sustained motion

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.print("BOOT OK");
  Wire.begin(21, 22);
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  setupBLE(); //Initialize BLE
  Serial.print("calling waitForConsent()");
  waitForConsent(); 
  calibrate();
}

#define SERVICE_UUID   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define POSTURE_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define COMMAND_UUID   "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

BLECharacteristic *postureCharacteristic;
BLECharacteristic *commandCharacteristic;

bool deviceConnected = false;
bool calibrationRequested = false;

// BLE SERVER CALLBACKS
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Phone connected!");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Phone disconnected.");
    pServer->getAdvertising()->start();
  }
};

// BLE COMMAND CALLBACK
class MyCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String command = pCharacteristic->getValue();
    Serial.print("Command received: ");
    Serial.println(command);
    if (command == "CALIBRATE") {
      Serial.println("Calibration requested by phone.");
      calibrationRequested = true;
    }
  }
};

// BLE SETUP
void setupBLE() {
  Serial.println("Starting BLE...");
  BLEDevice::init("BackBuddy");
  BLEServer *pServer =
      BLEDevice::createServer();

  pServer->setCallbacks(
      new MyServerCallbacks()
  );

  BLEService *pService =
      pServer->createService(SERVICE_UUID);   // Create BLE service

  postureCharacteristic =
      pService->createCharacteristic(
          POSTURE_UUID,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY
      );

  postureCharacteristic->addDescriptor(
      new BLE2902()
  );

  commandCharacteristic =
      pService->createCharacteristic(
          COMMAND_UUID,
          BLECharacteristic::PROPERTY_WRITE
      );

  commandCharacteristic->setCallbacks(
      new MyCommandCallbacks()
  );

  pService->start();   // Start BLE service

  // Start advertising
  BLEAdvertising *pAdvertising =
      BLEDevice::getAdvertising(); 

  pAdvertising->addServiceUUID(
      SERVICE_UUID
  );
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("--------------------------------");
  Serial.println("BLE advertising started.");
  Serial.println("Device name: BackBuddy");
  Serial.println("Waiting for phone...");
  Serial.println("--------------------------------");
}

float getTilt() {
  mySensor.accelUpdate();
  float aX = mySensor.accelX();
  float aY = mySensor.accelY();
  float aZ = mySensor.accelZ();
  return atan2(sqrt(aX*aX + aY*aY), aZ) * 180.0 / PI;
}

float getGyroMagnitude() {
  mySensor.gyroUpdate();
  float gX = mySensor.gyroX();
  float gY = mySensor.gyroY();
  float gZ = mySensor.gyroZ();
  return sqrt(gX*gX + gY*gY + gZ*gZ);
}

void waitForConsent() {
  Serial.println("BackBuddy is ready.");
  Serial.println("Connect using the phone app and press 'Start Calibration'.");

  while (true) {

    if (calibrationRequested) {
      Serial.println("Calibration request received!");
      calibrationRequested = false;
      break;
    }
    delay(10);
  }
}

void calibrate() {
  Serial.println("Sit upright. Calibrating in 3 seconds...");
  if (deviceConnected) {
    postureCharacteristic->setValue(
        "CALIBRATION_START"
    );
    postureCharacteristic->notify();
  }
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
  // Notify phone calibration is complete
  if (deviceConnected) {
    String message ="CALIBRATION_COMPLETE:" + String(baseline, 2);
    postureCharacteristic->setValue(
        message.c_str()
    );
    postureCharacteristic->notify();
  }
}

bool isActivelyMoving() {
  float gyroMag = getGyroMagnitude();

  if (gyroMag > gyroThreshold) {
    if (!sustainedMovement) {
      sustainedMovement = true;
      movementStartTime = millis();
    }
    // Only count as "really moving" once it's been elevated for a full second
    return (millis() - movementStartTime >= requiredMovementDuration);
  } else {
    sustainedMovement = false;
    return false;
  }
}
void loop() {
  if (!calibrated) {
    waitForConsent();
    calibrate();
    return;
  }
  float tilt = getTilt();
  float deviation = abs(tilt - baseline);  
  float gyroMag = getGyroMagnitude();

  if (deviation > slouchThreshold && !isActivelyMoving()) {
    if (!currentlySlouching) {
      currentlySlouching = true;
      slouchStartTime = millis();
    } else if (millis() - slouchStartTime >= requiredSlouchDuration) {
      Serial.println("SLOUCHING TOO LONG - VIBRATE!");
      digitalWrite(MOTOR_PIN, HIGH);  // vibrate
      delay(500);                      
      digitalWrite(MOTOR_PIN, LOW);   // stop
    }
  } else if (!isActivelyMoving()) {
    currentlySlouching = false;
  }
  if (deviceConnected) {
    String data = String(tilt, 2) + "," + String(deviation, 2) + "," + String(currentlySlouching ? 1 : 0);
    postureCharacteristic->setValue(
        data.c_str()
    );
    postureCharacteristic->notify();
  }

  Serial.print("Tilt: "); Serial.print(tilt, 2);
  Serial.print("  Deviation: "); Serial.print(deviation, 2);
  Serial.print("  GyroMag: "); Serial.print(gyroMag, 2);
  Serial.print("  Slouching: "); Serial.println(currentlySlouching ? "YES" : "NO");

  delay(1000);
}