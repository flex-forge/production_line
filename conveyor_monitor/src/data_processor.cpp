#include "data_processor.h"

DataProcessor::DataProcessor() {
  // Delegated constructors handle initialization
}

void DataProcessor::begin() {
  Serial.println(F("Initializing data processor..."));
  
  // Initialize specialized components
  statisticalAnalyzer.begin();
  anomalyDetector.begin();
  
  Serial.println(F("Data processor ready"));
}

void DataProcessor::update(const SystemState& state) {
  // Update statistical analysis first (provides data for anomaly detection)
  statisticalAnalyzer.update(state);
  
  // Update anomaly detection with current statistics
  anomalyDetector.update(state, 
                        statisticalAnalyzer.getAverageSpeed(),
                        statisticalAnalyzer.getSpeedVariance(),
                        statisticalAnalyzer.getVibrationBaseline());
}

bool DataProcessor::detectSpeedAnomaly() const {
  return anomalyDetector.detectSpeedAnomaly(statisticalAnalyzer.getAverageSpeed(),
                                           statisticalAnalyzer.getSpeedVariance());
}

bool DataProcessor::detectJam() const {
  return anomalyDetector.detectJam();
}

bool DataProcessor::detectVibrationAnomaly() const {
  return anomalyDetector.detectVibrationAnomaly(statisticalAnalyzer.getCurrentVibration(),
                                               statisticalAnalyzer.getVibrationBaseline(),
                                               statisticalAnalyzer.getVibrationTrend());
}

bool DataProcessor::detectEnvironmentalAnomaly() const {
  return anomalyDetector.detectEnvironmentalAnomaly(statisticalAnalyzer.getCurrentTemperature(),
                                                   statisticalAnalyzer.getCurrentHumidity(),
                                                   statisticalAnalyzer.getTemperatureVariance());
}