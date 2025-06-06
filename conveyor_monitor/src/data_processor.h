#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <Arduino.h>
#include "data_types.h"
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
  
  // Anomaly detection interface
  bool detectSpeedAnomaly() const;
  bool detectJam() const;
  bool detectVibrationAnomaly() const;
  bool detectEnvironmentalAnomaly() const;
  
  // Statistical analysis interface  
  float getAverageSpeed() const { return statisticalAnalyzer.getAverageSpeed(); }
  float getSpeedStability() const { return statisticalAnalyzer.getSpeedStability(); }
  float getVibrationTrend() const { return statisticalAnalyzer.getVibrationTrend(); }
  bool isJamDetected() const { return anomalyDetector.isJamDetected(); }
  
  // Advanced analytics
  float predictMaintenanceHours() const { return statisticalAnalyzer.predictMaintenanceHours(); }
  float getEfficiencyScore() const { return statisticalAnalyzer.getEfficiencyScore(anomalyDetector.isJamDetected()); }
  
  // Access to specialized components (if needed)
  const StatisticalAnalyzer& getStatisticalAnalyzer() const { return statisticalAnalyzer; }
  const AnomalyDetector& getAnomalyDetector() const { return anomalyDetector; }
};

#endif // DATA_PROCESSOR_H