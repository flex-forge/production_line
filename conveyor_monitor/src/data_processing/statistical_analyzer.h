#ifndef STATISTICAL_ANALYZER_H
#define STATISTICAL_ANALYZER_H

#include <Arduino.h>
#include "../config/data_types.h"
#include "../config/sensor_config.h"

/**
 * @brief Specialized class for statistical analysis of sensor data
 * 
 * This class handles data history management, statistical calculations,
 * and trend analysis for all monitored parameters.
 */
class StatisticalAnalyzer {
private:
  // Speed monitoring
  static const int SPEED_HISTORY_SIZE = 10;
  float speedHistory[SPEED_HISTORY_SIZE];
  int speedHistoryIndex;
  float averageSpeed;
  float speedVariance;
  
  // Vibration analysis
  static const int VIBRATION_HISTORY_SIZE = 30;
  float vibrationHistory[VIBRATION_HISTORY_SIZE];
  int vibrationHistoryIndex;
  float vibrationBaseline;
  bool baselineEstablished;
  
  // Environmental monitoring
  static const int ENV_HISTORY_SIZE = 10;
  float tempHistory[ENV_HISTORY_SIZE];
  float humidityHistory[ENV_HISTORY_SIZE];
  int envHistoryIndex;
  
  /**
   * @brief Calculate mean of array data
   * @param data Array of float values
   * @param size Size of array
   * @return Mean value
   */
  float calculateMean(const float* data, int size) const;
  
  /**
   * @brief Calculate variance of array data
   * @param data Array of float values
   * @param size Size of array
   * @param mean Pre-calculated mean value
   * @return Variance value
   */
  float calculateVariance(const float* data, int size, float mean) const;
  
  /**
   * @brief Calculate linear trend (slope) of array data
   * @param data Array of float values
   * @param size Size of array
   * @return Slope value indicating trend direction and magnitude
   */
  float calculateTrend(const float* data, int size) const;

public:
  /**
   * @brief Constructor
   */
  StatisticalAnalyzer();
  
  /**
   * @brief Initialize the statistical analyzer
   */
  void begin();
  
  /**
   * @brief Update statistics with new system state data
   * @param state Current system state
   */
  void update(const SystemState& state);
  
  // Speed statistics getters
  float getAverageSpeed() const { return averageSpeed; }
  float getSpeedVariance() const { return speedVariance; }
  float getSpeedStability() const { return speedVariance; } // Alias for compatibility
  
  // Vibration statistics getters
  float getVibrationBaseline() const { return vibrationBaseline; }
  float getVibrationTrend() const;
  bool isBaselineEstablished() const { return baselineEstablished; }
  float getCurrentVibration() const;
  
  // Environmental statistics getters
  float getTemperatureVariance() const;
  float getHumidityTrend() const;
  float getCurrentTemperature() const;
  float getCurrentHumidity() const;
  
  /**
   * @brief Get efficiency score based on statistical analysis
   * @param jamDetected Whether a jam is currently detected
   * @return Efficiency score from 0-100
   */
  float getEfficiencyScore(bool jamDetected) const;
  
  /**
   * @brief Predict maintenance hours based on vibration trend
   * @return Estimated hours until maintenance needed
   */
  float predictMaintenanceHours() const;
};

#endif // STATISTICAL_ANALYZER_H