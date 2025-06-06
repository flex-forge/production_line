#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "../config/config.h"

class SensorManager {
private:
  // Sensor objects will be initialized in implementation
  void* bme688;
  void* vl53l1x;
  void* lsm9ds1;
  void* apds9960;
  
  // Sensor availability flags
  bool seesawAvailable;
  bool bme688Available;
  bool vl53l1xAvailable;
  bool lsm9ds1Available;
  bool apds9960Available;
  
  // Encoder variables
  int32_t encoderPosition;
  int32_t baselineEncoderPosition;  // Position at startup (zero speed)
  unsigned long lastEncoderTime;
  float currentSpeed_rpm;
  
  // Part detection
  unsigned long lastPartDetectTime;
  int partCount;
  unsigned long partCountStartTime;
  
  // Vibration data
  float vibrationBuffer[VIBRATION_SAMPLE_SIZE];
  int vibrationBufferIndex;
  float vibrationMagnitude;
  
  // Gesture data
  GestureType lastGesture;
  unsigned long lastGestureTime;
  
  // Current readings
  SensorReadings currentReadings;
  
  // Private methods
  bool initializeSeesaw();
  bool initializeBME688();
  bool initializeVL53L1X();
  bool initializeLSM9DS1();
  bool initializeAPDS9960();
  
  /**
   * @brief Helper method for consistent sensor initialization handling
   * @param initFunc Pointer to sensor-specific initialization function
   * @param availabilityFlag Reference to the sensor availability flag
   * @param sensorName Human-readable sensor name for logging
   * @param virtualFallbackMsg Message to display when falling back to virtual data
   * @return true if sensor initialized successfully, false otherwise
   */
  bool initializeSensorWithFallback(bool (SensorManager::*initFunc)(), bool& availabilityFlag, 
                                   const char* sensorName, const char* virtualFallbackMsg);
  
  void readEncoder();
  void readEnvironmental();
  void readDistance();
  void readIMU();
  void readGesture();
  
  void calculateVibration();
  void updatePartCount();
  
  // Virtual sensor data generation
  void generateVirtualEncoderData();
  void generateVirtualEnvironmentalData();
  void generateVirtualDistanceData();
  void generateVirtualIMUData();
  void generateVirtualGestureData();
  
public:
  /**
   * @brief Constructor - initializes sensor manager with default values
   */
  SensorManager();
  
  /**
   * @brief Initialize all I2C sensors and configure hardware
   * @return true if all sensors initialized successfully, false if any failed
   * @note In virtual sensor mode, always returns true even if physical sensors fail
   */
  bool begin();
  
  /**
   * @brief Read all sensor data and update internal state
   * @details Performs non-blocking reads from all available sensors:
   *          - Rotary encoder (speed control)
   *          - BME688 environmental sensor
   *          - VL53L1X ToF distance sensor
   *          - LSM9DS1 IMU for vibration analysis
   *          - APDS9960 gesture and proximity sensor
   */
  void readAll();
  
  /**
   * @brief Get current conveyor speed in RPM
   * @return Speed in RPM (0-100), calculated from encoder position offset
   */
  float getConveyorSpeed() const { return currentSpeed_rpm; }
  
  /**
   * @brief Get parts per minute count based on object detection
   * @return Parts detected per minute, calculated from detection events
   */
  int getPartsCount() const;
  
  /**
   * @brief Get current vibration magnitude from IMU
   * @return RMS vibration magnitude in g-force units
   */
  float getVibrationMagnitude() const { return vibrationMagnitude; }
  
  /**
   * @brief Get ambient temperature reading
   * @return Temperature in degrees Celsius from BME688 sensor
   */
  float getTemperature() const { return currentReadings.temperature; }
  
  /**
   * @brief Get relative humidity reading
   * @return Humidity percentage (0-100%) from BME688 sensor
   */
  float getHumidity() const { return currentReadings.humidity; }
  
  /**
   * @brief Get atmospheric pressure reading
   * @return Pressure in hPa from BME688 sensor
   */
  float getPressure() const { return currentReadings.pressure; }
  
  /**
   * @brief Get air quality gas resistance reading
   * @return Gas resistance in Ohms from BME688 sensor (higher = better air quality)
   */
  uint32_t getAirQuality() const { return currentReadings.gasResistance; }
  
  /**
   * @brief Check if operator is present near the system
   * @return true if APDS9960 proximity sensor detects presence (>10 units)
   */
  bool isOperatorPresent() const { return currentReadings.proximity > 10; }
  
  /**
   * @brief Get the last detected gesture from operator
   * @return GestureType enum (UP, DOWN, LEFT, RIGHT, WAVE, or NONE)
   */
  GestureType getLastGesture() const { return lastGesture; }
  
  /**
   * @brief Get complete raw sensor readings structure
   * @return Reference to SensorReadings with all current sensor values
   */
  const SensorReadings& getRawReadings() const { return currentReadings; }
  
  /**
   * @brief Clear the last detected gesture (mark as processed)
   */
  void clearGesture() { lastGesture = GESTURE_NONE; }
  
  /**
   * @brief Check health status of all sensors
   * @return true if all sensors are responding correctly
   * @details Validates sensor availability and logs any communication errors
   */
  bool checkSensorHealth() const;
};


#endif // SENSOR_MANAGER_H