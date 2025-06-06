#include "anomaly_detector.h"

AnomalyDetector::AnomalyDetector() {
  lowVibrationStartTime = 0;
  wasRunning = false;
  inLowVibrationState = false;
  
  // Calculate thresholds once
  speedToleranceRPM = NOMINAL_SPEED_RPM * (SPEED_TOLERANCE_PCT / 100.0f);
  vibrationWarningLevel = VIBRATION_WARNING_G;
  vibrationCriticalLevel = VIBRATION_CRITICAL_G;
}

void AnomalyDetector::begin() {
  lowVibrationStartTime = millis();
  Serial.println(F("Anomaly detector initialized"));
}

void AnomalyDetector::update(const SystemState& state, float averageSpeed, float speedVariance, float vibrationBaseline) {
  // Vibration-based jam detection
  unsigned long currentTime = millis();
  
  // Check if belt should be running but vibration is too low
  if (state.conveyorRunning && state.speed_rpm > MIN_SPEED_THRESHOLD) {
    if (state.vibrationLevel < JAM_VIBRATION_THRESHOLD) {
      // Low vibration while running - potential jam
      if (!inLowVibrationState) {
        // Just entered low vibration state
        lowVibrationStartTime = currentTime;
        inLowVibrationState = true;
        Serial.print(F("Jam detection: Low vibration detected ("));
        Serial.print(state.vibrationLevel);
        Serial.print(F("g < "));
        Serial.print(JAM_VIBRATION_THRESHOLD);
        Serial.println(F("g) while running"));
      } else {
        // Check if we've been in low vibration state long enough
        if (currentTime - lowVibrationStartTime > JAM_DETECT_TIME_MS) {
          // Jam confirmed - vibration has been low for too long
          static unsigned long lastJamMsg = 0;
          if (currentTime - lastJamMsg > 5000) { // Don't spam
            Serial.println(F("JAM DETECTED: Low vibration for extended period"));
            lastJamMsg = currentTime;
          }
        }
      }
    } else {
      // Vibration is normal - reset jam detection
      if (inLowVibrationState) {
        Serial.println(F("Jam detection: Vibration returned to normal"));
      }
      inLowVibrationState = false;
      lowVibrationStartTime = currentTime;
    }
  } else {
    // Belt is not supposed to be running - reset jam detection
    inLowVibrationState = false;
    lowVibrationStartTime = currentTime;
  }
  
  wasRunning = state.conveyorRunning;
}

bool AnomalyDetector::detectSpeedAnomaly(float averageSpeed, float speedVariance) const {
  // Check if speed deviates from nominal by more than tolerance
  if (averageSpeed < MIN_SPEED_THRESHOLD) {
    return false; // Conveyor is stopped, not an anomaly
  }
  
  float deviation = abs(averageSpeed - NOMINAL_SPEED_RPM);
  
  // Also check for high variance (unstable speed)
  bool speedUnstable = speedVariance > (speedToleranceRPM * 0.5f);
  
  return (deviation > speedToleranceRPM) || speedUnstable;
}

bool AnomalyDetector::detectJam() const {
  // Vibration-based jam detection
  // Jam is detected if belt should be running but vibration is too low for extended period
  unsigned long currentTime = millis();
  return inLowVibrationState && (currentTime - lowVibrationStartTime > JAM_DETECT_TIME_MS);
}

bool AnomalyDetector::detectVibrationAnomaly(float currentVibration, float vibrationBaseline, float vibrationTrend) const {
  // Critical threshold check
  if (currentVibration > vibrationCriticalLevel) {
    return true;
  }
  
  // Warning if above baseline and trending up
  if (currentVibration > vibrationWarningLevel && vibrationTrend > 0.01f) {
    return true;
  }
  
  return false;
}

bool AnomalyDetector::detectEnvironmentalAnomaly(float temperature, float humidity, float tempVariance) const {
  // Check temperature bounds
  if (temperature < TEMP_MIN_C || temperature > TEMP_MAX_C) {
    return true;
  }
  
  // Check humidity
  if (humidity > HUMIDITY_MAX_PCT) {
    return true;
  }
  
  // Check for rapid temperature changes
  if (tempVariance > 5.0f) { // Rapid temperature change
    return true;
  }
  
  return false;
}

unsigned long AnomalyDetector::getJamDuration() const {
  if (!inLowVibrationState) {
    return 0;
  }
  return millis() - lowVibrationStartTime;
}