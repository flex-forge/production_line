#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <Arduino.h>
#include "../config/data_types.h"
#include "anomaly_detector.h"
#include "statistical_analyzer.h"

/**
 * @brief Main data processing coordinator
 * 
 * This class coordinates between statistical analysis and anomaly detection,
 * providing a unified interface for the main application. It delegates
 * specialized tasks to the appropriate analyzer classes.
 */
class DataProcessor {
private:
  StatisticalAnalyzer statisticalAnalyzer;
  AnomalyDetector anomalyDetector;
  
public:
  /**
   * @brief Constructor
   */
  DataProcessor();
  
  /**
   * @brief Initialize the data processor and its components
   */
  void begin();
  
  /**
   * @brief Update all analysis components with new system state
   * @param state Current system state
   */
  void update(const SystemState& state);
  
  /**
   * @brief Detect speed anomalies in conveyor operation
   * @return true if speed deviation exceeds tolerance or shows instability
   * @details Checks for deviations >10% from nominal 60 RPM and high variance
   */
  bool detectSpeedAnomaly() const;
  
  /**
   * @brief Detect conveyor jam conditions
   * @return true if jam detected (low vibration while belt should be running)
   * @details Uses vibration-based detection: <0.3g for >10 seconds while speed >5 RPM
   */
  bool detectJam() const;
  
  /**
   * @brief Detect abnormal vibration patterns
   * @return true if vibration exceeds critical thresholds or shows concerning trends
   * @details Checks for >2.0g critical level or >1.0g with upward trend
   */
  bool detectVibrationAnomaly() const;
  
  /**
   * @brief Detect environmental condition anomalies
   * @return true if temperature, humidity, or pressure outside operating ranges
   * @details Monitors temp (10-40°C), humidity (<80%), rapid changes (>5°C variance)
   */
  bool detectEnvironmentalAnomaly() const;
  
  /**
   * @brief Get running average conveyor speed
   * @return Average speed in RPM over last 10 readings
   */
  float getAverageSpeed() const { return statisticalAnalyzer.getAverageSpeed(); }
  
  /**
   * @brief Get speed stability metric (variance)
   * @return Speed variance indicating stability (lower = more stable)
   */
  float getSpeedStability() const { return statisticalAnalyzer.getSpeedStability(); }
  
  /**
   * @brief Get vibration trend analysis
   * @return Linear slope indicating vibration trend (positive = increasing)
   */
  float getVibrationTrend() const { return statisticalAnalyzer.getVibrationTrend(); }
  
  /**
   * @brief Check if jam condition is currently active
   * @return true if system is in active jam state
   */
  bool isJamDetected() const { return anomalyDetector.isJamDetected(); }
  
  /**
   * @brief Predict hours until maintenance needed
   * @return Estimated hours until maintenance required based on vibration trends
   * @details Returns 999.0 if no degradation trend detected
   */
  float predictMaintenanceHours() const { return statisticalAnalyzer.predictMaintenanceHours(); }
  
  /**
   * @brief Calculate overall system efficiency score
   * @return Efficiency percentage (0-100) based on speed, vibration, and jam status
   * @details Weighted average: 40% speed + 40% vibration + 20% jam penalty
   */
  float getEfficiencyScore() const { return statisticalAnalyzer.getEfficiencyScore(anomalyDetector.isJamDetected()); }
  
  /**
   * @brief Get direct access to statistical analyzer component
   * @return Reference to StatisticalAnalyzer for advanced analysis
   */
  const StatisticalAnalyzer& getStatisticalAnalyzer() const { return statisticalAnalyzer; }
  
  /**
   * @brief Get direct access to anomaly detector component
   * @return Reference to AnomalyDetector for advanced anomaly analysis
   */
  const AnomalyDetector& getAnomalyDetector() const { return anomalyDetector; }
};

#endif // DATA_PROCESSOR_H