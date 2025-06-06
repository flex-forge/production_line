#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>

// System state structure
struct SystemState {
  bool conveyorRunning;
  float speed_rpm;
  int partsPerMinute;
  float vibrationLevel;
  float temperature;
  float humidity;
  float pressure;
  uint32_t gasResistance;
  unsigned long lastJamTime;
  bool operatorPresent;
};

// Sensor reading structure
struct SensorReadings {
  // Encoder
  float encoderSpeed;
  int32_t encoderPulses;

  // ToF
  uint16_t distance_mm;
  bool objectDetected;

  // IMU
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float mag_x, mag_y, mag_z;

  // Environmental
  float temperature;
  float humidity;
  float pressure;
  uint32_t gasResistance;

  // Gesture
  uint8_t gesture;
  uint8_t proximity;
};

#endif // DATA_TYPES_H