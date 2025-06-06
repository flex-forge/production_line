#include "sensor_manager.h"
#include "error_handling.h"
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
  baselineEncoderPosition = 0;
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

bool SensorManager::initializeSensorWithFallback(bool (SensorManager::*initFunc)(), bool& availabilityFlag, 
                                                const char* sensorName, const char* virtualFallbackMsg) {
  availabilityFlag = (this->*initFunc)();
  if (!availabilityFlag) {
    Serial.print(sensorName);
    Serial.println(F(" init failed"));
    LOG_ERROR_CTX(SystemError::SENSOR_INIT_FAILED, sensorName);
    
    #if VIRTUAL_SENSOR
      Serial.print(F("  -> Using "));
      Serial.println(virtualFallbackMsg);
    #endif
  } else {
    Serial.print(sensorName);
    Serial.println(F(" initialized successfully"));
  }
  return availabilityFlag;
}

bool SensorManager::begin() {
  Serial.println(F("Initializing sensors..."));
  bool allSensorsOk = true;

  // Initialize I2C sensors using helper method to reduce duplication
  allSensorsOk &= initializeSensorWithFallback(&SensorManager::initializeSeesaw, seesawAvailable, 
                                              "Seesaw encoder", "virtual encoder data");
  
  allSensorsOk &= initializeSensorWithFallback(&SensorManager::initializeBME688, bme688Available, 
                                              "BME688", "virtual environmental data");
  
  allSensorsOk &= initializeSensorWithFallback(&SensorManager::initializeVL53L1X, vl53l1xAvailable, 
                                              "VL53L1X", "virtual distance data");
  
  allSensorsOk &= initializeSensorWithFallback(&SensorManager::initializeLSM9DS1, lsm9ds1Available, 
                                              "LSM9DS1", "virtual IMU data");
  
  allSensorsOk &= initializeSensorWithFallback(&SensorManager::initializeAPDS9960, apds9960Available, 
                                              "APDS9960", "virtual gesture data");

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
  
  // Read current position as baseline (zero speed position)
  baselineEncoderPosition = seesaw.getEncoderPosition();
  encoderPosition = baselineEncoderPosition;
  lastEncoderTime = millis();
  
  Serial.print(F("Encoder baseline position set to: "));
  Serial.println(baselineEncoderPosition);
  
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
  Serial.println(F("Initializing VL53L1X ToF sensor..."));
  
  // Set longer timeout for initialization
  distanceSensor.setTimeout(2000);
  
  if (!distanceSensor.init()) {
    Serial.println(F("VL53L1X init() failed"));
    return false;
  }
  
  // Configure for short range, fast operation
  distanceSensor.setDistanceMode(VL53L1X::Short);
  distanceSensor.setMeasurementTimingBudget(50000); // 50ms timing budget
  
  // Start continuous mode
  distanceSensor.startContinuous(100); // 100ms between measurements
  
  // Wait for first measurement to be ready
  delay(200);
  
  // Test initial reading with timeout check
  uint16_t testDistance = distanceSensor.read(false);
  if (testDistance == 0 || distanceSensor.timeoutOccurred()) {
    Serial.println(F("VL53L1X initial read failed or timeout"));
    // Don't fail initialization - sensor might work in normal operation
    Serial.println(F("VL53L1X proceeding anyway - will use in continuous mode"));
  } else {
    Serial.print(F("VL53L1X test distance: "));
    Serial.print(testDistance);
    Serial.println(F("mm"));
  }
  
  Serial.println(F("VL53L1X initialization complete"));
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
  gestureSensor.enableProximitySensor(true);  // Enable proximity for gesture detection
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
    
    // Calculate speed based on offset from baseline position
    int32_t positionOffset = encoderPosition - baselineEncoderPosition;
    
    // Convert position offset to speed (each detent = 1 RPM increment)
    // Positive offset = faster, negative offset = reverse/slower
    currentSpeed_rpm = positionOffset * 1.0; // 1 RPM per detent

    // Clamp speed to reasonable range
    if (currentSpeed_rpm < 0.0) currentSpeed_rpm = 0.0;        // No negative speeds
    if (currentSpeed_rpm > 100.0) currentSpeed_rpm = 100.0;    // Max 100 RPM

    // Debug encoder readings
    /*
    static unsigned long lastEncoderDebug = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastEncoderDebug > 5000) { // Every 5 seconds
      Serial.print(F("Encoder - Position: "));
      Serial.print(encoderPosition);
      Serial.print(F(", Baseline: "));
      Serial.print(baselineEncoderPosition);
      Serial.print(F(", Offset: "));
      Serial.print(positionOffset);
      Serial.print(F(", Speed: "));
      Serial.print(currentSpeed_rpm);
      Serial.println(F(" RPM"));
      lastEncoderDebug = currentTime;
    }
    */

    currentReadings.encoderSpeed = currentSpeed_rpm;
    currentReadings.encoderPulses = encoderPosition;
  } else {
    Serial.println(F("Seesaw not available, using virtual encoder"));
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
    
    if (distance != 0 && !distanceSensor.timeoutOccurred()) {
      currentReadings.distance_mm = distance;
      
      // Detect if object is present
      currentReadings.objectDetected = (distance < PART_DETECT_THRESHOLD);
      
      // Track part detection
      if (currentReadings.objectDetected && !lastPartDetected) {
        partCount++;
        lastPartDetectTime = millis();
      }
      lastPartDetected = currentReadings.objectDetected;
    } else {
      // Handle timeout - keep last valid reading but don't update part detection
      if (distanceSensor.timeoutOccurred()) {
        Serial.println(F("VL53L1X timeout"));
      }
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
            Serial.println(F("Gesture detected: UP"));
            break;
          case DIR_DOWN:
            lastGesture = GESTURE_SWIPE_DOWN;
            Serial.println(F("Gesture detected: DOWN"));
            break;
          case DIR_LEFT:
            lastGesture = GESTURE_SWIPE_LEFT;
            Serial.println(F("Gesture detected: LEFT"));
            break;
          case DIR_RIGHT:
            lastGesture = GESTURE_SWIPE_RIGHT;
            Serial.println(F("Gesture detected: RIGHT"));
            break;
          case DIR_NEAR:
          case DIR_FAR:
            lastGesture = GESTURE_WAVE;
            Serial.println(F("Gesture detected: WAVE"));
            break;
        }
        lastGestureTime = currentTime;
      }
    }
    
    // Read proximity
    uint8_t proximity = 0;
    gestureSensor.readProximity(proximity);
    currentReadings.proximity = proximity;
    
    // Periodic proximity debug
    static unsigned long lastProxDebug = 0;
    if (millis() - lastProxDebug > 10000) { // Every 10 seconds
      Serial.print(F("APDS9960 Proximity: "));
      Serial.print(proximity);
      Serial.print(F(" (Operator: "));
      Serial.print(proximity > 10 ? "YES" : "NO");
      Serial.println(F(")"));
      lastProxDebug = millis();
    }
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

int SensorManager::getPartsCount() const {
  // Calculate parts per minute
  unsigned long elapsed = millis() - partCountStartTime;
  if (elapsed > 0) {
    return (partCount * 60000) / elapsed;
  }
  return 0;
}

bool SensorManager::checkSensorHealth() const {
  // Check if we're getting readings from all sensors
  bool healthy = true;
  
  // Check if encoder is responding (position-based speed control always responds)
  if (seesawAvailable) {
    // Note: We can't check position changes in a const method
    // This is acceptable as we primarily check availability flags
  }
  
  // Check ToF sensor (read-only health check)
  if (vl53l1xAvailable) {
    // For const method, we primarily check availability
    // Actual timeout checking happens during regular reads
  } else {
    LOG_ERROR_CTX(SystemError::SENSOR_READ_TIMEOUT, "VL53L1X not available");
    healthy = false;
  }
  
  // Check BME688 availability
  if (!bme688Available) {
    LOG_ERROR_CTX(SystemError::SENSOR_READ_TIMEOUT, "BME688 not available");
    healthy = false;
  }
  
  // Check IMU availability
  if (!lsm9ds1Available) {
    LOG_ERROR_CTX(SystemError::SENSOR_READ_TIMEOUT, "LSM9DS1 not available");
    healthy = false;
  }
  
  // Check gesture sensor availability
  if (!apds9960Available) {
    LOG_ERROR_CTX(SystemError::SENSOR_READ_TIMEOUT, "APDS9960 not available");
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

