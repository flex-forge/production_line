#ifndef ANOMALY_DETECTOR_H
#define ANOMALY_DETECTOR_H

#include <Arduino.h>
#include "data_types.h"
#include "sensor_config.h"

/**
 * @brief Specialized class for detecting system anomalies
 * 
 * This class focuses specifically on anomaly detection algorithms
 * including speed deviations, vibration anomalies, environmental
 * conditions, and jam detection.
 */
class AnomalyDetector {
private:
  // Jam detection state
  unsigned long lowVibrationStartTime;
  bool wasRunning;
  bool inLowVibrationState;
  
  // Thresholds for detection
  float speedToleranceRPM;
  float vibrationWarningLevel;
  float vibrationCriticalLevel;
  
public:
  /**
   * @brief Constructor
   */
  AnomalyDetector();
  
  /**
   * @brief Initialize the anomaly detector
   */
  void begin();
  
  /**
   * @brief Update detector state with current system data
   * @param state Current system state
   * @param averageSpeed Running average speed from statistics
   * @param speedVariance Speed variance from statistics
   * @param vibrationBaseline Established vibration baseline
   */
  void update(const SystemState& state, float averageSpeed, float speedVariance, float vibrationBaseline);
  
  /**
   * @brief Detect speed anomalies
   * @param averageSpeed Current average speed
   * @param speedVariance Current speed variance
   * @return true if anomaly detected
   */
  bool detectSpeedAnomaly(float averageSpeed, float speedVariance) const;
  
  /**
   * @brief Detect conveyor jam using vibration analysis
   * @return true if jam detected
   */
  bool detectJam() const;
  
  /**
   * @brief Detect vibration anomalies
   * @param currentVibration Current vibration level
   * @param vibrationBaseline Established baseline
   * @param vibrationTrend Trend over time
   * @return true if anomaly detected
   */
  bool detectVibrationAnomaly(float currentVibration, float vibrationBaseline, float vibrationTrend) const;
  
  /**
   * @brief Detect environmental anomalies
   * @param temperature Current temperature
   * @param humidity Current humidity
   * @param tempVariance Temperature variance over time
   * @return true if anomaly detected
   */
  bool detectEnvironmentalAnomaly(float temperature, float humidity, float tempVariance) const;
  
  /**
   * @brief Check if currently in jam state
   * @return true if jam state active
   */
  bool isJamDetected() const { return inLowVibrationState; }
  
  /**
   * @brief Get time since jam detection started
   * @return milliseconds since jam detection started, 0 if no jam
   */
  unsigned long getJamDuration() const;
};

#endif // ANOMALY_DETECTOR_H