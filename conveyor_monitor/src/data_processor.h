#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <Arduino.h>
#include "config.h"

class DataProcessor {
private:
  // Speed monitoring
  float speedHistory[10];
  int speedHistoryIndex;
  float averageSpeed;
  float speedVariance;
  
  // Jam detection (vibration-based)
  unsigned long lowVibrationStartTime;
  bool wasRunning;
  bool inLowVibrationState;
  
  // Vibration analysis
  float vibrationBaseline;
  float vibrationHistory[30];
  int vibrationHistoryIndex;
  bool baselineEstablished;
  
  // Environmental monitoring
  float tempHistory[10];
  float humidityHistory[10];
  int envHistoryIndex;
  
  // Statistical functions
  float calculateMean(float* data, int size);
  float calculateVariance(float* data, int size, float mean);
  float calculateTrend(float* data, int size);
  
public:
  DataProcessor();
  
  void begin();
  void update(const SystemState& state);
  
  // Anomaly detection
  bool detectSpeedAnomaly();
  bool detectJam();
  bool detectVibrationAnomaly();
  bool detectEnvironmentalAnomaly();
  
  // Get processed metrics
  float getAverageSpeed() { return averageSpeed; }
  float getSpeedStability() { return speedVariance; }
  float getVibrationTrend();
  bool isJamDetected() { return inLowVibrationState; }
  
  // Get predictions
  float predictMaintenanceHours();
  float getEfficiencyScore();
};

#endif // DATA_PROCESSOR_H