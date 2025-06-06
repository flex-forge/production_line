#include "telemetry_formatter.h"
#include "error_handling.h"

TelemetryFormatter::TelemetryFormatter() {
  // Constructor - no initialization needed
}

float TelemetryFormatter::validateFloat(float value, float defaultValue) const {
  if (isnan(value) || !isfinite(value)) {
    return defaultValue;
  }
  return value;
}

void TelemetryFormatter::appendFloatField(String& json, const char* fieldName, float value, int precision, bool isLast) const {
  json += "\"";
  json += fieldName;
  json += "\":";
  json += String(value, precision);
  if (!isLast) {
    json += ",";
  }
}

void TelemetryFormatter::appendIntField(String& json, const char* fieldName, int value, bool isLast) const {
  json += "\"";
  json += fieldName;
  json += "\":";
  json += String(value);
  if (!isLast) {
    json += ",";
  }
}

void TelemetryFormatter::appendBoolField(String& json, const char* fieldName, bool value, bool isLast) const {
  json += "\"";
  json += fieldName;
  json += "\":";
  json += value ? "true" : "false";
  if (!isLast) {
    json += ",";
  }
}

bool TelemetryFormatter::formatTelemetry(const SystemState& state, char* outputBuffer, size_t bufferSize) const {
  if (outputBuffer == nullptr || bufferSize == 0) {
    LOG_ERROR(SystemError::INVALID_PARAMETER);
    return false;
  }
  
  // Validate and sanitize input data
  float speed = validateFloat(state.speed_rpm, 0.0f);
  float vibration = validateFloat(state.vibrationLevel, 0.0f);
  float temp = validateFloat(state.temperature, 22.0f);
  float humidity = validateFloat(state.humidity, 50.0f);
  float pressure = validateFloat(state.pressure, 1013.25f);
  uint32_t gas = state.gasResistance;
  
  // Build JSON string using String concatenation for STM32 compatibility
  String telemetryStr = "{";
  
  appendFloatField(telemetryStr, "speed_rpm", speed, 1, false);
  appendIntField(telemetryStr, "parts_per_min", state.partsPerMinute, false);
  appendFloatField(telemetryStr, "vibration", vibration, 2, false);
  appendFloatField(telemetryStr, "temp", temp, 1, false);
  appendFloatField(telemetryStr, "humidity", humidity, 1, false);
  appendFloatField(telemetryStr, "pressure", pressure, 1, false);
  appendIntField(telemetryStr, "gas_resistance", gas, false);
  appendBoolField(telemetryStr, "running", state.conveyorRunning, false);
  appendBoolField(telemetryStr, "operator", state.operatorPresent, true);
  
  telemetryStr += "}";
  
  // Check if the result fits in the output buffer
  if (telemetryStr.length() >= bufferSize) {
    Serial.println(F("ERROR: Telemetry string too large for buffer"));
    LOG_ERROR(SystemError::BUFFER_OVERFLOW);
    return false;
  }
  
  // Copy to output buffer
  telemetryStr.toCharArray(outputBuffer, bufferSize);
  return true;
}

bool TelemetryFormatter::validateSystemState(const SystemState& state) const {
  bool isValid = true;
  
  // Check for invalid float values
  if (isnan(state.speed_rpm) || !isfinite(state.speed_rpm)) {
    Serial.println(F("WARNING: Invalid speed_rpm value"));
    isValid = false;
  }
  
  if (isnan(state.vibrationLevel) || !isfinite(state.vibrationLevel)) {
    Serial.println(F("WARNING: Invalid vibrationLevel value"));
    isValid = false;
  }
  
  if (isnan(state.temperature) || !isfinite(state.temperature)) {
    Serial.println(F("WARNING: Invalid temperature value"));
    isValid = false;
  }
  
  if (isnan(state.humidity) || !isfinite(state.humidity)) {
    Serial.println(F("WARNING: Invalid humidity value"));
    isValid = false;
  }
  
  if (isnan(state.pressure) || !isfinite(state.pressure)) {
    Serial.println(F("WARNING: Invalid pressure value"));
    isValid = false;
  }
  
  // Check for reasonable value ranges
  if (state.speed_rpm < 0.0f || state.speed_rpm > 200.0f) {
    Serial.print(F("WARNING: Speed out of reasonable range: "));
    Serial.println(state.speed_rpm);
  }
  
  if (state.temperature < -50.0f || state.temperature > 100.0f) {
    Serial.print(F("WARNING: Temperature out of reasonable range: "));
    Serial.println(state.temperature);
  }
  
  return isValid;
}

void TelemetryFormatter::printDebugInfo(const SystemState& state) const {
  Serial.println(F("=== System State Debug ==="));
  Serial.print(F("Speed: ")); Serial.print(state.speed_rpm); Serial.println(F(" RPM"));
  Serial.print(F("Parts/min: ")); Serial.println(state.partsPerMinute);
  Serial.print(F("Vibration: ")); Serial.print(state.vibrationLevel); Serial.println(F(" g"));
  Serial.print(F("Temperature: ")); Serial.print(state.temperature); Serial.println(F(" °C"));
  Serial.print(F("Humidity: ")); Serial.print(state.humidity); Serial.println(F(" %"));
  Serial.print(F("Pressure: ")); Serial.print(state.pressure); Serial.println(F(" hPa"));
  Serial.print(F("Gas Resistance: ")); Serial.print(state.gasResistance); Serial.println(F(" Ω"));
  Serial.print(F("Running: ")); Serial.println(state.conveyorRunning ? "YES" : "NO");
  Serial.print(F("Operator: ")); Serial.println(state.operatorPresent ? "YES" : "NO");
}