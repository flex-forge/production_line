#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "config.h"

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
  SensorManager();
  
  // Initialize all sensors
  bool begin();
  
  // Read all sensors
  void readAll();
  
  // Get processed values (const-correct)
  float getConveyorSpeed() const { return currentSpeed_rpm; }
  int getPartsCount() const;
  float getVibrationMagnitude() const { return vibrationMagnitude; }
  float getTemperature() const { return currentReadings.temperature; }
  float getHumidity() const { return currentReadings.humidity; }
  float getPressure() const { return currentReadings.pressure; }
  uint32_t getAirQuality() const { return currentReadings.gasResistance; }
  bool isOperatorPresent() const { return currentReadings.proximity > 10; }
  GestureType getLastGesture() const { return lastGesture; }
  
  // Get raw sensor data
  const SensorReadings& getRawReadings() const { return currentReadings; }
  
  // Utility functions
  void clearGesture() { lastGesture = GESTURE_NONE; }
  bool checkSensorHealth() const;
};


#endif // SENSOR_MANAGER_H