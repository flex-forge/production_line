#include "statistical_analyzer.h"

StatisticalAnalyzer::StatisticalAnalyzer() {
  speedHistoryIndex = 0;
  averageSpeed = 0.0f;
  speedVariance = 0.0f;
  vibrationHistoryIndex = 0;
  vibrationBaseline = VIBRATION_BASELINE_G;
  baselineEstablished = false;
  envHistoryIndex = 0;
}

void StatisticalAnalyzer::begin() {
  // Initialize history arrays
  for (int i = 0; i < SPEED_HISTORY_SIZE; i++) {
    speedHistory[i] = 0.0f;
  }
  
  for (int i = 0; i < ENV_HISTORY_SIZE; i++) {
    tempHistory[i] = 20.0f; // Assume room temp
    humidityHistory[i] = 50.0f; // Assume normal humidity
  }
  
  for (int i = 0; i < VIBRATION_HISTORY_SIZE; i++) {
    vibrationHistory[i] = VIBRATION_BASELINE_G;
  }
  
  Serial.println(F("Statistical analyzer initialized"));
}

void StatisticalAnalyzer::update(const SystemState& state) {
  // Update speed history
  speedHistory[speedHistoryIndex] = state.speed_rpm;
  speedHistoryIndex = (speedHistoryIndex + 1) % SPEED_HISTORY_SIZE;
  
  // Calculate speed statistics
  averageSpeed = calculateMean(speedHistory, SPEED_HISTORY_SIZE);
  speedVariance = calculateVariance(speedHistory, SPEED_HISTORY_SIZE, averageSpeed);
  
  // Update vibration history
  vibrationHistory[vibrationHistoryIndex] = state.vibrationLevel;
  vibrationHistoryIndex = (vibrationHistoryIndex + 1) % VIBRATION_HISTORY_SIZE;
  
  // Establish vibration baseline after full cycle
  if (!baselineEstablished && vibrationHistoryIndex == 0) {
    vibrationBaseline = calculateMean(vibrationHistory, VIBRATION_HISTORY_SIZE);
    baselineEstablished = true;
    Serial.print(F("Vibration baseline established: "));
    Serial.print(vibrationBaseline);
    Serial.println(F("g"));
  }
  
  // Update environmental history
  tempHistory[envHistoryIndex] = state.temperature;
  humidityHistory[envHistoryIndex] = state.humidity;
  envHistoryIndex = (envHistoryIndex + 1) % ENV_HISTORY_SIZE;
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
  if (!baselineEstablished) {
    return 0.0f;
  }
  return calculateTrend(vibrationHistory, VIBRATION_HISTORY_SIZE);
}

float StatisticalAnalyzer::getCurrentVibration() const {
  int currentIndex = (vibrationHistoryIndex - 1 + VIBRATION_HISTORY_SIZE) % VIBRATION_HISTORY_SIZE;
  return vibrationHistory[currentIndex];
}

float StatisticalAnalyzer::getTemperatureVariance() const {
  float mean = calculateMean(tempHistory, ENV_HISTORY_SIZE);
  return calculateVariance(tempHistory, ENV_HISTORY_SIZE, mean);
}

float StatisticalAnalyzer::getHumidityTrend() const {
  return calculateTrend(humidityHistory, ENV_HISTORY_SIZE);
}

float StatisticalAnalyzer::getCurrentTemperature() const {
  int currentIndex = (envHistoryIndex - 1 + ENV_HISTORY_SIZE) % ENV_HISTORY_SIZE;
  return tempHistory[currentIndex];
}

float StatisticalAnalyzer::getCurrentHumidity() const {
  int currentIndex = (envHistoryIndex - 1 + ENV_HISTORY_SIZE) % ENV_HISTORY_SIZE;
  return humidityHistory[currentIndex];
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