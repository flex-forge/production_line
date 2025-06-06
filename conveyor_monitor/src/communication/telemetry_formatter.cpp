#include "telemetry_formatter.h"
#include "../utils/error_handling.h"
#include "../utils/performance_utils.h"

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
  
  // Use fast string builder for better performance
  FastStringBuilder builder(outputBuffer, bufferSize);
  
  builder.append("{\"speed_rpm\":")
         .append(speed, 1)
         .append(",\"parts_per_min\":")
         .appendUInt(state.partsPerMinute)
         .append(",\"vibration\":")
         .append(vibration, 2)
         .append(",\"temp\":")
         .append(temp, 1)
         .append(",\"humidity\":")
         .append(humidity, 1)
         .append(",\"pressure\":")
         .append(pressure, 1)
         .append(",\"gas_resistance\":")
         .appendUInt(gas)
         .append(",\"running\":")
         .append(state.conveyorRunning)
         .append(",\"operator\":")
         .append(state.operatorPresent)
         .append("}");
  
  // Check if we ran out of space
  if (builder.getLength() >= bufferSize - 1) {
    Serial.println(F("ERROR: Telemetry string too large for buffer"));
    LOG_ERROR(SystemError::BUFFER_OVERFLOW);
    return false;
  }
  
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