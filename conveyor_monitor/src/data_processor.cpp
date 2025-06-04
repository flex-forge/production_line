#include "data_processor.h"

DataProcessor::DataProcessor() {
  speedHistoryIndex = 0;
  averageSpeed = 0.0;
  speedVariance = 0.0;
  lastMovementTime = 0;
  lastDistance = 0;
  stuckPartCount = 0;
  vibrationBaseline = VIBRATION_BASELINE_G;
  vibrationHistoryIndex = 0;
  baselineEstablished = false;
  envHistoryIndex = 0;
}

void DataProcessor::begin() {
  // Initialize history arrays
  for (int i = 0; i < 10; i++) {
    speedHistory[i] = 0.0;
    tempHistory[i] = 20.0; // Assume room temp
    humidityHistory[i] = 50.0; // Assume normal humidity
  }
  
  for (int i = 0; i < 30; i++) {
    vibrationHistory[i] = VIBRATION_BASELINE_G;
  }
  
  lastMovementTime = millis();
}

void DataProcessor::update(const SystemState& state) {
  // Update speed history
  speedHistory[speedHistoryIndex] = state.speed_rpm;
  speedHistoryIndex = (speedHistoryIndex + 1) % 10;
  
  // Calculate speed statistics
  averageSpeed = calculateMean(speedHistory, 10);
  speedVariance = calculateVariance(speedHistory, 10, averageSpeed);
  
  // Update vibration history
  vibrationHistory[vibrationHistoryIndex] = state.vibrationLevel;
  vibrationHistoryIndex = (vibrationHistoryIndex + 1) % 30;
  
  // Establish vibration baseline after 30 samples
  if (!baselineEstablished && vibrationHistoryIndex == 0) {
    vibrationBaseline = calculateMean(vibrationHistory, 30);
    baselineEstablished = true;
  }
  
  // Update environmental history
  tempHistory[envHistoryIndex] = state.temperature;
  humidityHistory[envHistoryIndex] = state.humidity;
  envHistoryIndex = (envHistoryIndex + 1) % 10;
  
  // Check for movement (jam detection)
  if (state.conveyorRunning && state.partsPerMinute == 0) {
    if (millis() - lastMovementTime > JAM_DETECT_TIME_MS) {
      stuckPartCount++;
    }
  } else {
    lastMovementTime = millis();
    stuckPartCount = 0;
  }
}

bool DataProcessor::detectSpeedAnomaly() {
  // Check if speed deviates from nominal by more than tolerance
  if (averageSpeed < MIN_SPEED_THRESHOLD) {
    return false; // Conveyor is stopped, not an anomaly
  }
  
  float deviation = abs(averageSpeed - NOMINAL_SPEED_RPM);
  float toleranceRPM = NOMINAL_SPEED_RPM * (SPEED_TOLERANCE_PCT / 100.0);
  
  // Also check for high variance (unstable speed)
  bool speedUnstable = speedVariance > (toleranceRPM * 0.5);
  
  return (deviation > toleranceRPM) || speedUnstable;
}

bool DataProcessor::detectJam() {
  // Jam is detected if parts aren't moving for extended period
  return (stuckPartCount > 3);
}

bool DataProcessor::detectVibrationAnomaly() {
  if (!baselineEstablished) {
    return false; // Need baseline first
  }
  
  // Check current vibration against thresholds
  float currentVibration = vibrationHistory[(vibrationHistoryIndex - 1 + 30) % 30];
  
  // Critical threshold check
  if (currentVibration > VIBRATION_CRITICAL_G) {
    return true;
  }
  
  // Check trend - increasing vibration over time
  float trend = calculateTrend(vibrationHistory, 30);
  float recentAvg = calculateMean(vibrationHistory, 10); // Last 10 samples
  
  // Warning if above baseline and trending up
  if (recentAvg > VIBRATION_WARNING_G && trend > 0.01) {
    return true;
  }
  
  return false;
}

bool DataProcessor::detectEnvironmentalAnomaly() {
  float currentTemp = tempHistory[(envHistoryIndex - 1 + 10) % 10];
  float currentHumidity = humidityHistory[(envHistoryIndex - 1 + 10) % 10];
  
  // Check temperature bounds
  if (currentTemp < TEMP_MIN_C || currentTemp > TEMP_MAX_C) {
    return true;
  }
  
  // Check humidity
  if (currentHumidity > HUMIDITY_MAX_PCT) {
    return true;
  }
  
  // Check for rapid changes
  float tempVariance = calculateVariance(tempHistory, 10, calculateMean(tempHistory, 10));
  if (tempVariance > 5.0) { // Rapid temperature change
    return true;
  }
  
  return false;
}

float DataProcessor::calculateMean(float* data, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum / size;
}

float DataProcessor::calculateVariance(float* data, int size, float mean) {
  float sumSquares = 0;
  for (int i = 0; i < size; i++) {
    sumSquares += sq(data[i] - mean);
  }
  return sumSquares / size;
}

float DataProcessor::calculateTrend(float* data, int size) {
  // Simple linear regression slope
  float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
  
  for (int i = 0; i < size; i++) {
    sumX += i;
    sumY += data[i];
    sumXY += i * data[i];
    sumX2 += i * i;
  }
  
  float slope = (size * sumXY - sumX * sumY) / (size * sumX2 - sumX * sumX);
  return slope;
}

float DataProcessor::getVibrationTrend() {
  if (!baselineEstablished) {
    return 0.0;
  }
  return calculateTrend(vibrationHistory, 30);
}

float DataProcessor::predictMaintenanceHours() {
  // Simple prediction based on vibration trend
  if (!baselineEstablished) {
    return 999.0; // Unknown
  }
  
  float trend = getVibrationTrend();
  float currentVibration = vibrationHistory[(vibrationHistoryIndex - 1 + 30) % 30];
  
  if (trend <= 0) {
    return 999.0; // No degradation
  }
  
  // Estimate hours until critical threshold
  float remainingG = VIBRATION_CRITICAL_G - currentVibration;
  float hoursToFailure = (remainingG / trend) * 24; // Assuming trend is per day
  
  return max(0.0f, hoursToFailure);
}

float DataProcessor::getEfficiencyScore() {
  // Calculate efficiency based on multiple factors
  float speedScore = 100.0;
  float vibrationScore = 100.0;
  float jamScore = 100.0;
  
  // Speed efficiency
  if (averageSpeed > 0) {
    float speedRatio = averageSpeed / NOMINAL_SPEED_RPM;
    speedScore = 100.0 * (1.0 - abs(1.0 - speedRatio));
  }
  
  // Vibration score
  if (baselineEstablished) {
    float currentVibration = vibrationHistory[(vibrationHistoryIndex - 1 + 30) % 30];
    vibrationScore = 100.0 * (1.0 - (currentVibration / VIBRATION_CRITICAL_G));
  }
  
  // Jam score (penalize recent jams)
  if (stuckPartCount > 0) {
    jamScore = max(0.0f, 100.0f - (stuckPartCount * 20.0f));
  }
  
  // Weighted average
  return (speedScore * 0.4 + vibrationScore * 0.4 + jamScore * 0.2);
}