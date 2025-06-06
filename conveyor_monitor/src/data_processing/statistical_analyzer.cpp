#include "statistical_analyzer.h"

StatisticalAnalyzer::StatisticalAnalyzer() 
  : speedHistory(true), vibrationHistory(true), tempHistory(true), humidityHistory(true) {
  averageSpeed = 0.0f;
  speedVariance = 0.0f;
  vibrationBaseline = VIBRATION_BASELINE_G;
  baselineEstablished = false;
}

void StatisticalAnalyzer::begin() {
  // Clear all circular buffers
  speedHistory.clear();
  vibrationHistory.clear();
  tempHistory.clear();
  humidityHistory.clear();
  
  // Pre-fill buffers with reasonable default values
  for (int i = 0; i < 10; i++) {
    speedHistory.push(0.0f);
    tempHistory.push(20.0f);  // Room temperature
    humidityHistory.push(50.0f);  // Normal humidity
  }
  
  for (int i = 0; i < 30; i++) {
    vibrationHistory.push(VIBRATION_BASELINE_G);
  }
  
  Serial.println(F("Statistical analyzer initialized"));
}

void StatisticalAnalyzer::update(const SystemState& state) {
  // Update speed history using circular buffer
  speedHistory.push(state.speed_rpm);
  
  // Calculate speed statistics using circular buffer methods
  averageSpeed = speedHistory.average();
  speedVariance = speedHistory.variance(averageSpeed);
  
  // Update vibration history using circular buffer
  vibrationHistory.push(state.vibrationLevel);
  
  // Establish vibration baseline after buffer is full
  if (!baselineEstablished && vibrationHistory.isFull()) {
    vibrationBaseline = vibrationHistory.average();
    baselineEstablished = true;
    Serial.print(F("Vibration baseline established: "));
    Serial.print(vibrationBaseline);
    Serial.println(F("g"));
  }
  
  // Update environmental history using circular buffers
  tempHistory.push(state.temperature);
  humidityHistory.push(state.humidity);
}

float StatisticalAnalyzer::calculateMean(const float* data, int size) const {
  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum / size;
}

float StatisticalAnalyzer::calculateVariance(const float* data, int size, float mean) const {
  float sumSquares = 0.0f;
  for (int i = 0; i < size; i++) {
    float diff = data[i] - mean;
    sumSquares += diff * diff;
  }
  return sumSquares / size;
}

float StatisticalAnalyzer::calculateTrend(const float* data, int size) const {
  // Simple linear regression slope
  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
  
  for (int i = 0; i < size; i++) {
    float x = static_cast<float>(i);
    float y = data[i];
    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }
  
  float denominator = size * sumX2 - sumX * sumX;
  if (abs(denominator) < 0.001f) {
    return 0.0f; // Avoid division by zero
  }
  
  float slope = (size * sumXY - sumX * sumY) / denominator;
  return slope;
}

float StatisticalAnalyzer::getVibrationTrend() const {
  if (!baselineEstablished || vibrationHistory.size() < 2) {
    return 0.0f;
  }
  
  // Calculate linear trend using least squares method on circular buffer
  size_t size = vibrationHistory.size();
  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
  
  for (size_t i = 0; i < size; i++) {
    float x = static_cast<float>(i);
    float y = vibrationHistory[i];
    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }
  
  float denominator = size * sumX2 - sumX * sumX;
  if (abs(denominator) < 0.001f) {
    return 0.0f; // Avoid division by zero
  }
  
  return (size * sumXY - sumX * sumY) / denominator;
}

float StatisticalAnalyzer::getCurrentVibration() const {
  if (vibrationHistory.isEmpty()) {
    return 0.0f;
  }
  return vibrationHistory.newest();
}

float StatisticalAnalyzer::getTemperatureVariance() const {
  if (tempHistory.isEmpty()) {
    return 0.0f;
  }
  float mean = tempHistory.average();
  return tempHistory.variance(mean);
}

float StatisticalAnalyzer::getHumidityTrend() const {
  if (humidityHistory.size() < 2) {
    return 0.0f;
  }
  
  // Calculate linear trend for humidity
  size_t size = humidityHistory.size();
  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
  
  for (size_t i = 0; i < size; i++) {
    float x = static_cast<float>(i);
    float y = humidityHistory[i];
    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }
  
  float denominator = size * sumX2 - sumX * sumX;
  if (abs(denominator) < 0.001f) {
    return 0.0f; // Avoid division by zero
  }
  
  return (size * sumXY - sumX * sumY) / denominator;
}

float StatisticalAnalyzer::getCurrentTemperature() const {
  if (tempHistory.isEmpty()) {
    return 20.0f; // Default room temperature
  }
  return tempHistory.newest();
}

float StatisticalAnalyzer::getCurrentHumidity() const {
  if (humidityHistory.isEmpty()) {
    return 50.0f; // Default humidity
  }
  return humidityHistory.newest();
}

float StatisticalAnalyzer::getEfficiencyScore(bool jamDetected) const {
  // Calculate efficiency based on multiple factors
  float speedScore = 100.0f;
  float vibrationScore = 100.0f;
  float jamScore = 100.0f;
  
  // Speed efficiency
  if (averageSpeed > 0.0f) {
    float speedRatio = averageSpeed / NOMINAL_SPEED_RPM;
    speedScore = 100.0f * (1.0f - abs(1.0f - speedRatio));
    speedScore = max(0.0f, min(100.0f, speedScore)); // Clamp to 0-100
  }
  
  // Vibration score
  if (baselineEstablished) {
    float currentVibration = getCurrentVibration();
    vibrationScore = 100.0f * (1.0f - (currentVibration / VIBRATION_CRITICAL_G));
    vibrationScore = max(0.0f, min(100.0f, vibrationScore)); // Clamp to 0-100
  }
  
  // Jam score (penalize current jam state)
  if (jamDetected) {
    jamScore = 0.0f; // Significant penalty for active jam
  }
  
  // Weighted average
  return (speedScore * 0.4f + vibrationScore * 0.4f + jamScore * 0.2f);
}

float StatisticalAnalyzer::predictMaintenanceHours() const {
  // Simple prediction based on vibration trend
  if (!baselineEstablished) {
    return 999.0f; // Unknown
  }
  
  float trend = getVibrationTrend();
  float currentVibration = getCurrentVibration();
  
  if (trend <= 0.0f) {
    return 999.0f; // No degradation
  }
  
  // Estimate hours until critical threshold
  float remainingG = VIBRATION_CRITICAL_G - currentVibration;
  float hoursToFailure = (remainingG / trend) * 24.0f; // Assuming trend is per day
  
  return max(0.0f, hoursToFailure);
}