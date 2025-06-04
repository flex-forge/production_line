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
  
  // Encoder variables
  int32_t encoderPosition;
  int32_t lastEncoderPosition;
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
  
  void readEncoder();
  void readEnvironmental();
  void readDistance();
  void readIMU();
  void readGesture();
  
  void calculateVibration();
  void updatePartCount();
  
public:
  SensorManager();
  
  // Initialize all sensors
  bool begin();
  
  // Read all sensors
  void readAll();
  
  // Get processed values
  float getConveyorSpeed() { return currentSpeed_rpm; }
  int getPartsCount();
  float getVibrationMagnitude() { return vibrationMagnitude; }
  float getTemperature() { return currentReadings.temperature; }
  float getHumidity() { return currentReadings.humidity; }
  float getPressure() { return currentReadings.pressure; }
  uint32_t getAirQuality() { return currentReadings.gasResistance; }
  bool isOperatorPresent() { return currentReadings.proximity > 10; }
  GestureType getLastGesture() { return lastGesture; }
  
  // Get raw sensor data
  SensorReadings getRawReadings() { return currentReadings; }
  
  // Utility functions
  void clearGesture() { lastGesture = GESTURE_NONE; }
  bool checkSensorHealth();
};


#endif // SENSOR_MANAGER_H