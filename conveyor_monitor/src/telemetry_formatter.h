#ifndef TELEMETRY_FORMATTER_H
#define TELEMETRY_FORMATTER_H

#include <Arduino.h>
#include "data_types.h"

/**
 * @brief Handles telemetry data formatting and validation
 * 
 * This class encapsulates the logic for converting system state data
 * into JSON format for cloud transmission, including data validation
 * and sanitization.
 */
class TelemetryFormatter {
private:
  static const size_t TELEMETRY_BUFFER_SIZE = 512;
  
  /**
   * @brief Validates and sanitizes a float value
   * @param value The float value to validate
   * @param defaultValue The default value to use if invalid
   * @return Validated float value
   */
  float validateFloat(float value, float defaultValue) const;
  
  /**
   * @brief Appends a float field to the JSON string
   * @param json The JSON string to append to
   * @param fieldName The field name
   * @param value The float value
   * @param precision Number of decimal places
   * @param isLast Whether this is the last field (no comma)
   */
  void appendFloatField(String& json, const char* fieldName, float value, int precision, bool isLast = false) const;
  
  /**
   * @brief Appends an integer field to the JSON string
   * @param json The JSON string to append to
   * @param fieldName The field name
   * @param value The integer value
   * @param isLast Whether this is the last field (no comma)
   */
  void appendIntField(String& json, const char* fieldName, int value, bool isLast = false) const;
  
  /**
   * @brief Appends a boolean field to the JSON string
   * @param json The JSON string to append to
   * @param fieldName The field name
   * @param value The boolean value
   * @param isLast Whether this is the last field (no comma)
   */
  void appendBoolField(String& json, const char* fieldName, bool value, bool isLast = false) const;

public:
  /**
   * @brief Constructor
   */
  TelemetryFormatter();
  
  /**
   * @brief Formats system state data into JSON telemetry string
   * @param state The system state data
   * @param outputBuffer The buffer to write the JSON string to
   * @param bufferSize The size of the output buffer
   * @return true if formatting succeeded, false otherwise
   */
  bool formatTelemetry(const SystemState& state, char* outputBuffer, size_t bufferSize) const;
  
  /**
   * @brief Validates system state data and logs any issues
   * @param state The system state data to validate
   * @return true if all data is valid, false if issues found
   */
  bool validateSystemState(const SystemState& state) const;
  
  /**
   * @brief Prints debug information about the system state
   * @param state The system state data
   */
  void printDebugInfo(const SystemState& state) const;
};

#endif // TELEMETRY_FORMATTER_H