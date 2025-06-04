#include "sensor_manager.h"
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <VL53L1X.h>
#include <SparkFunLSM9DS1.h>
#include <SparkFun_APDS9960.h>

// Variable to track last part detection state
static bool lastPartDetected = false;

// Global instance pointer for ISR
SensorManager* sensorManagerInstance = nullptr;

// Sensor objects
Adafruit_BME680 bme;
VL53L1X distanceSensor;
LSM9DS1 imu;
SparkFun_APDS9960 gestureSensor;

// Encoder ISR
void encoderISRHandler() {
  if (sensorManagerInstance) {
    sensorManagerInstance->encoderISR();
  }
}

SensorManager::SensorManager() {
  encoderPulseCount = 0;
  lastEncoderTime = 0;
  lastPulseCount = 0;
  currentSpeed_rpm = 0.0;
  lastPartDetectTime = 0;
  partCount = 0;
  partCountStartTime = millis();
  vibrationBufferIndex = 0;
  vibrationMagnitude = 0.0;
  lastGesture = GESTURE_NONE;
  lastGestureTime = 0;
  sensorManagerInstance = this;
}

bool SensorManager::begin() {
  Serial.println(F("Initializing sensors..."));
  
  // Initialize encoder
  initializeEncoder();
  
  // Initialize I2C sensors
  if (!initializeBME688()) {
    Serial.println(F("BME688 init failed"));
    return false;
  }
  
  if (!initializeVL53L1X()) {
    Serial.println(F("VL53L1X init failed"));
    return false;
  }
  
  if (!initializeLSM9DS1()) {
    Serial.println(F("LSM9DS1 init failed"));
    return false;
  }
  
  if (!initializeAPDS9960()) {
    Serial.println(F("APDS9960 init failed"));
    return false;
  }
  
  Serial.println(F("All sensors initialized successfully"));
  return true;
}

void SensorManager::initializeEncoder() {
  // Configure encoder pin as input with pullup
  pinMode(2, INPUT_PULLUP); // Assuming encoder on pin 2
  attachInterrupt(digitalPinToInterrupt(2), encoderISRHandler, RISING);
  lastEncoderTime = millis();
}

bool SensorManager::initializeBME688() {
  if (!bme.begin(BME688_I2C_ADDR)) {
    return false;
  }
  
  // Configure BME688
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
  
  return true;
}

bool SensorManager::initializeVL53L1X() {
  distanceSensor.setTimeout(500);
  
  if (!distanceSensor.init()) {
    return false;
  }
  
  // Configure for short range, high speed
  distanceSensor.setDistanceMode(VL53L1X::Short);
  distanceSensor.setMeasurementTimingBudget(20000); // 20ms
  distanceSensor.startContinuous(50); // 50ms between measurements
  
  return true;
}

bool SensorManager::initializeLSM9DS1() {
  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = LSM9DS1_M_I2C_ADDR;
  imu.settings.device.agAddress = LSM9DS1_AG_I2C_ADDR;
  
  if (!imu.begin()) {
    return false;
  }
  
  // Configure for vibration detection
  imu.settings.accel.scale = 8; // +/- 8g
  imu.settings.accel.sampleRate = 5; // 119Hz
  imu.settings.gyro.scale = 245; // 245 dps
  imu.settings.gyro.sampleRate = 5; // 119Hz
  
  return true;
}

bool SensorManager::initializeAPDS9960() {
  if (!gestureSensor.init()) {
    return false;
  }
  
  // Configure gesture sensor
  gestureSensor.enableGestureSensor(true);
  gestureSensor.enableProximitySensor(false);
  gestureSensor.setGestureGain(GGAIN_2X);
  gestureSensor.setGestureLEDDrive(LED_DRIVE_25MA);
  
  return true;
}

void SensorManager::readAll() {
  readEncoder();
  readEnvironmental();
  readDistance();
  readIMU();
  readGesture();
  
  calculateVibration();
  updatePartCount();
}

void SensorManager::readEncoder() {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastEncoderTime;
  
  if (timeDiff >= 1000) { // Calculate speed every second
    unsigned long pulseDiff = encoderPulseCount - lastPulseCount;
    
    // Calculate RPM: (pulses/sec) * 60 / (pulses/rev) / gear_ratio
    currentSpeed_rpm = (pulseDiff * 60000.0) / (timeDiff * ENCODER_PULSES_PER_REV * CONVEYOR_GEAR_RATIO);
    
    lastPulseCount = encoderPulseCount;
    lastEncoderTime = currentTime;
    
    currentReadings.encoderSpeed = currentSpeed_rpm;
    currentReadings.encoderPulses = encoderPulseCount;
  }
}

void SensorManager::readEnvironmental() {
  if (bme.performReading()) {
    currentReadings.temperature = bme.temperature;
    currentReadings.humidity = bme.humidity;
    currentReadings.pressure = bme.pressure / 100.0; // Convert to hPa
    currentReadings.gasResistance = bme.gas_resistance;
  }
}

void SensorManager::readDistance() {
  uint16_t distance = distanceSensor.read(false);
  
  if (distance != 0) { // 0 indicates timeout
    currentReadings.distance_mm = distance;
    
    // Detect if object is present
    currentReadings.objectDetected = (distance < PART_DETECT_THRESHOLD);
    
    // Track part detection
    if (currentReadings.objectDetected && !lastPartDetected) {
      partCount++;
      lastPartDetectTime = millis();
    }
    lastPartDetected = currentReadings.objectDetected;
  }
}

void SensorManager::readIMU() {
  if (imu.accelAvailable()) {
    imu.readAccel();
    currentReadings.accel_x = imu.calcAccel(imu.ax);
    currentReadings.accel_y = imu.calcAccel(imu.ay);
    currentReadings.accel_z = imu.calcAccel(imu.az);
    
    // Add to vibration buffer
    float magnitude = sqrt(sq(currentReadings.accel_x) + 
                          sq(currentReadings.accel_y) + 
                          sq(currentReadings.accel_z));
    
    vibrationBuffer[vibrationBufferIndex] = magnitude;
    vibrationBufferIndex = (vibrationBufferIndex + 1) % VIBRATION_SAMPLE_SIZE;
  }
  
  if (imu.gyroAvailable()) {
    imu.readGyro();
    currentReadings.gyro_x = imu.calcGyro(imu.gx);
    currentReadings.gyro_y = imu.calcGyro(imu.gy);
    currentReadings.gyro_z = imu.calcGyro(imu.gz);
  }
  
  if (imu.magAvailable()) {
    imu.readMag();
    currentReadings.mag_x = imu.calcMag(imu.mx);
    currentReadings.mag_y = imu.calcMag(imu.my);
    currentReadings.mag_z = imu.calcMag(imu.mz);
  }
}

void SensorManager::readGesture() {
  if (gestureSensor.isGestureAvailable()) {
    int gesture = gestureSensor.readGesture();
    unsigned long currentTime = millis();
    
    // Apply cooldown to prevent spam
    if (currentTime - lastGestureTime >= GESTURE_COOLDOWN_MS) {
      switch (gesture) {
        case DIR_UP:
          lastGesture = GESTURE_SWIPE_UP;
          break;
        case DIR_DOWN:
          lastGesture = GESTURE_SWIPE_DOWN;
          break;
        case DIR_LEFT:
          lastGesture = GESTURE_SWIPE_LEFT;
          break;
        case DIR_RIGHT:
          lastGesture = GESTURE_SWIPE_RIGHT;
          break;
        case DIR_NEAR:
        case DIR_FAR:
          lastGesture = GESTURE_WAVE;
          break;
      }
      lastGestureTime = currentTime;
    }
  }
  
  // Read proximity
  uint8_t proximity = 0;
  gestureSensor.readProximity(proximity);
  currentReadings.proximity = proximity;
}

void SensorManager::calculateVibration() {
  // Calculate RMS vibration magnitude
  float sum = 0;
  for (int i = 0; i < VIBRATION_SAMPLE_SIZE; i++) {
    sum += sq(vibrationBuffer[i]);
  }
  vibrationMagnitude = sqrt(sum / VIBRATION_SAMPLE_SIZE);
}

void SensorManager::updatePartCount() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - partCountStartTime;
  
  if (elapsed >= 60000) { // Reset counter every minute
    partCount = 0;
    partCountStartTime = currentTime;
  }
}

int SensorManager::getPartsCount() {
  // Calculate parts per minute
  unsigned long elapsed = millis() - partCountStartTime;
  if (elapsed > 0) {
    return (partCount * 60000) / elapsed;
  }
  return 0;
}

bool SensorManager::checkSensorHealth() {
  // Check if we're getting readings from all sensors
  bool healthy = true;
  
  // Check if encoder is responding (should have pulses if running)
  if (currentSpeed_rpm > MIN_SPEED_THRESHOLD && encoderPulseCount == lastPulseCount) {
    Serial.println(F("Encoder not responding"));
    healthy = false;
  }
  
  // Check ToF sensor
  if (distanceSensor.read(false) == 0) {
    Serial.println(F("ToF sensor timeout"));
    healthy = false;
  }
  
  // BME688 has internal error checking
  if (!bme.performReading()) {
    Serial.println(F("BME688 read error"));
    healthy = false;
  }
  
  return healthy;
}

void SensorManager::encoderISR() {
  encoderPulseCount++;
}