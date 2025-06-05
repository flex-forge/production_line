#include "sensor_manager.h"
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <VL53L1X.h>
#include <SparkFunLSM9DS1.h>
#include <SparkFun_APDS9960.h>
#include <Adafruit_seesaw.h>

// Variable to track last part detection state
static bool lastPartDetected = false;

// Sensor objects
Adafruit_BME680 bme;
VL53L1X distanceSensor;
LSM9DS1 imu;
SparkFun_APDS9960 gestureSensor;
Adafruit_seesaw seesaw;


SensorManager::SensorManager() {
  encoderPosition = 0;
  lastEncoderPosition = 0;
  lastEncoderTime = 0;
  currentSpeed_rpm = 0.0;
  lastPartDetectTime = 0;
  partCount = 0;
  partCountStartTime = millis();
  vibrationBufferIndex = 0;
  vibrationMagnitude = 0.0;
  lastGesture = GESTURE_NONE;
  lastGestureTime = 0;
  
  // Initialize availability flags
  seesawAvailable = false;
  bme688Available = false;
  vl53l1xAvailable = false;
  lsm9ds1Available = false;
  apds9960Available = false;
}

bool SensorManager::begin() {
  Serial.println(F("Initializing sensors..."));
  bool allSensorsOk = true;

  // Initialize I2C sensors
  seesawAvailable = initializeSeesaw();
  if (!seesawAvailable) {
    Serial.println(F("Seesaw encoder init failed"));
    #if VIRTUAL_SENSOR
      Serial.println(F("  -> Using virtual encoder data"));
    #else
      allSensorsOk = false;
    #endif
  }

  bme688Available = initializeBME688();
  if (!bme688Available) {
    Serial.println(F("BME688 init failed"));
    #if VIRTUAL_SENSOR
      Serial.println(F("  -> Using virtual environmental data"));
    #else
      allSensorsOk = false;
    #endif
  }

  vl53l1xAvailable = initializeVL53L1X();
  if (!vl53l1xAvailable) {
    Serial.println(F("VL53L1X init failed"));
    #if VIRTUAL_SENSOR
      Serial.println(F("  -> Using virtual distance data"));
    #else
      allSensorsOk = false;
    #endif
  }

  lsm9ds1Available = initializeLSM9DS1();
  if (!lsm9ds1Available) {
    Serial.println(F("LSM9DS1 init failed"));
    #if VIRTUAL_SENSOR
      Serial.println(F("  -> Using virtual IMU data"));
    #else
      allSensorsOk = false;
    #endif
  }

  apds9960Available = initializeAPDS9960();
  if (!apds9960Available) {
    Serial.println(F("APDS9960 init failed"));
    #if VIRTUAL_SENSOR
      Serial.println(F("  -> Using virtual gesture data"));
    #else
      allSensorsOk = false;
    #endif
  }

  #if VIRTUAL_SENSOR
    Serial.println(F("Sensor initialization complete (virtual mode enabled)"));
    return true;
  #else
    if (allSensorsOk) {
      Serial.println(F("All sensors initialized successfully"));
    }
    return allSensorsOk;
  #endif
}

bool SensorManager::initializeSeesaw() {
  if (!seesaw.begin(SEESAW_I2C_ADDR)) {
    return false;
  }

  // Get product info to verify correct Seesaw board
  uint32_t version = ((uint32_t)seesaw.getVersion() >> 16) & 0xFFFF;
  if (version != 4991) { // Check for rotary encoder product ID
    Serial.println(F("Wrong Seesaw product detected"));
    return false;
  }
  
  // Configure encoder module
  seesaw.pinMode(24, INPUT_PULLUP); // Button pin
  seesaw.setGPIOInterrupts((uint32_t)1 << 24, 1);
  seesaw.enableEncoderInterrupt();
  
  // Reset encoder position
  seesaw.setEncoderPosition(0);
  encoderPosition = 0;
  lastEncoderPosition = 0;
  lastEncoderTime = millis();
  
  return true;
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
  if (seesawAvailable) {
    // Read current encoder position
    encoderPosition = seesaw.getEncoderPosition();
    
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastEncoderTime;
    
    if (timeDiff >= 1000) { // Calculate speed every second
      int32_t positionDiff = encoderPosition - lastEncoderPosition;
      
      // Handle wraparound
      if (abs(positionDiff) > 32768) {
        if (positionDiff > 0) {
          positionDiff -= 65536;
        } else {
          positionDiff += 65536;
        }
      }
      
      // Calculate RPM: (pulses/sec) * 60 / (pulses/rev) / gear_ratio
      currentSpeed_rpm = (abs(positionDiff) * 60000.0) / (timeDiff * ENCODER_PULSES_PER_REV * CONVEYOR_GEAR_RATIO);
      
      lastEncoderPosition = encoderPosition;
      lastEncoderTime = currentTime;
      
      currentReadings.encoderSpeed = currentSpeed_rpm;
      currentReadings.encoderPulses = encoderPosition;
    }
  } else {
    generateVirtualEncoderData();
  }
}

void SensorManager::readEnvironmental() {
  if (bme688Available) {
    if (bme.performReading()) {
      currentReadings.temperature = bme.temperature;
      currentReadings.humidity = bme.humidity;
      currentReadings.pressure = bme.pressure / 100.0; // Convert to hPa
      currentReadings.gasResistance = bme.gas_resistance;
    }
  } else {
    generateVirtualEnvironmentalData();
  }
}

void SensorManager::readDistance() {
  if (vl53l1xAvailable) {
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
  } else {
    generateVirtualDistanceData();
  }
}

void SensorManager::readIMU() {
  if (lsm9ds1Available) {
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
  } else {
    generateVirtualIMUData();
  }
}

void SensorManager::readGesture() {
  if (apds9960Available) {
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
  } else {
    generateVirtualGestureData();
  }
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
  
  // Check if encoder is responding (should have position changes if running)
  if (seesawAvailable && currentSpeed_rpm > MIN_SPEED_THRESHOLD && encoderPosition == lastEncoderPosition) {
    Serial.println(F("Encoder not responding"));
    healthy = false;
  }
  
  // Check ToF sensor
  if (vl53l1xAvailable && distanceSensor.read(false) == 0) {
    Serial.println(F("ToF sensor timeout"));
    healthy = false;
  }
  
  // BME688 has internal error checking
  if (bme688Available && !bme.performReading()) {
    Serial.println(F("BME688 read error"));
    healthy = false;
  }
  
  return healthy;
}

// Virtual data generation methods
void SensorManager::generateVirtualEncoderData() {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastEncoderTime;
  
  if (timeDiff >= 1000) {
    // Simulate encoder with slight variations
    float variation = (sin(currentTime / 5000.0) * 2.0) - 1.0; // -1 to +1
    currentSpeed_rpm = NOMINAL_SPEED_RPM + variation;
    
    // Update position based on speed
    int32_t positionDiff = (currentSpeed_rpm * ENCODER_PULSES_PER_REV * CONVEYOR_GEAR_RATIO) / 60;
    encoderPosition += positionDiff;
    
    lastEncoderTime = currentTime;
    currentReadings.encoderSpeed = currentSpeed_rpm;
    currentReadings.encoderPulses = encoderPosition;
  }
}

void SensorManager::generateVirtualEnvironmentalData() {
  unsigned long currentTime = millis();
  
  // Generate realistic environmental data with slight variations
  float tempVariation = sin(currentTime / 30000.0) * 2.0; // ±2°C
  float humidityVariation = cos(currentTime / 45000.0) * 5.0; // ±5%
  
  currentReadings.temperature = 22.0 + tempVariation;
  currentReadings.humidity = 45.0 + humidityVariation;
  currentReadings.pressure = 1013.25 + (sin(currentTime / 60000.0) * 2.0);
  currentReadings.gasResistance = 150000 + (sin(currentTime / 20000.0) * 25000);
}

void SensorManager::generateVirtualDistanceData() {
  unsigned long currentTime = millis();
  
  // Simulate parts passing by
  float sineWave = sin(currentTime / 1000.0); // 1 Hz
  currentReadings.distance_mm = 200 + (sineWave * 150); // 50-350mm range
  
  // Detect virtual parts
  currentReadings.objectDetected = (currentReadings.distance_mm < PART_DETECT_THRESHOLD);
  
  // Track part detection
  if (currentReadings.objectDetected && !lastPartDetected) {
    partCount++;
    lastPartDetectTime = currentTime;
  }
  lastPartDetected = currentReadings.objectDetected;
}

void SensorManager::generateVirtualIMUData() {
  unsigned long currentTime = millis();
  
  // Generate realistic vibration data
  float baseVibration = VIBRATION_BASELINE_G;
  float vibrationNoise = (random(100) - 50) / 500.0; // ±0.1g noise
  float periodicVibration = sin(currentTime / 200.0) * 0.05; // 5Hz oscillation
  
  currentReadings.accel_x = vibrationNoise;
  currentReadings.accel_y = vibrationNoise * 0.8;
  currentReadings.accel_z = 1.0 + periodicVibration; // 1g gravity + vibration
  
  // Add to vibration buffer
  float magnitude = sqrt(sq(currentReadings.accel_x) + 
                        sq(currentReadings.accel_y) + 
                        sq(currentReadings.accel_z));
  
  vibrationBuffer[vibrationBufferIndex] = magnitude;
  vibrationBufferIndex = (vibrationBufferIndex + 1) % VIBRATION_SAMPLE_SIZE;
  
  // Simulate gyro (mostly zeros with small drift)
  currentReadings.gyro_x = (random(10) - 5) / 10.0;
  currentReadings.gyro_y = (random(10) - 5) / 10.0;
  currentReadings.gyro_z = (random(10) - 5) / 10.0;
  
  // Simulate magnetometer (Earth's field)
  currentReadings.mag_x = 25.0 + (random(10) - 5) / 5.0;
  currentReadings.mag_y = -5.0 + (random(10) - 5) / 5.0;
  currentReadings.mag_z = 45.0 + (random(10) - 5) / 5.0;
}

void SensorManager::generateVirtualGestureData() {
  // Simulate occasional operator presence
  unsigned long currentTime = millis();
  
  // Random proximity (operator near/far)
  if ((currentTime / 10000) % 3 == 0) { // Every 30 seconds
    currentReadings.proximity = 50 + random(100);
  } else {
    currentReadings.proximity = random(10);
  }
  
  // No gestures in virtual mode unless testing
  currentReadings.gesture = GESTURE_NONE;
}

