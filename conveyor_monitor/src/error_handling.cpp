#include "error_handling.h"

// Global error handler instance
ErrorHandler systemErrorHandler;

ErrorHandler::ErrorHandler() {
  errorHistoryIndex = 0;
  errorCount = 0;
  
  // Initialize arrays
  for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
    errorHistory[i] = SystemError::NONE;
    errorTimestamps[i] = 0;
  }
}

const char* ErrorHandler::getErrorString(SystemError error) const {
  switch (error) {
    case SystemError::NONE: return "No error";
    case SystemError::SENSOR_INIT_FAILED: return "Sensor initialization failed";
    case SystemError::SENSOR_READ_TIMEOUT: return "Sensor read timeout";
    case SystemError::SENSOR_DATA_INVALID: return "Invalid sensor data";
    case SystemError::I2C_COMMUNICATION_ERROR: return "I2C communication error";
    case SystemError::MEMORY_ALLOCATION_ERROR: return "Memory allocation error";
    case SystemError::NOTECARD_INIT_FAILED: return "Notecard initialization failed";
    case SystemError::NOTECARD_SEND_FAILED: return "Notecard send failed";
    case SystemError::CONFIG_VALIDATION_ERROR: return "Configuration validation error";
    case SystemError::TELEMETRY_FORMAT_ERROR: return "Telemetry formatting error";
    case SystemError::BUFFER_OVERFLOW: return "Buffer overflow";
    case SystemError::INVALID_PARAMETER: return "Invalid parameter";
    default: return "Unknown error";
  }
}

const char* ErrorHandler::getSeverityString(ErrorSeverity severity) const {
  switch (severity) {
    case ErrorSeverity::INFO: return "INFO";
    case ErrorSeverity::WARNING: return "WARNING";
    case ErrorSeverity::ERROR: return "ERROR";
    case ErrorSeverity::CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

ErrorSeverity getDefaultSeverity(SystemError error) {
  switch (error) {
    case SystemError::NONE:
      return ErrorSeverity::INFO;
    case SystemError::SENSOR_DATA_INVALID:
    case SystemError::TELEMETRY_FORMAT_ERROR:
      return ErrorSeverity::WARNING;
    case SystemError::SENSOR_READ_TIMEOUT:
    case SystemError::I2C_COMMUNICATION_ERROR:
    case SystemError::NOTECARD_SEND_FAILED:
    case SystemError::CONFIG_VALIDATION_ERROR:
    case SystemError::BUFFER_OVERFLOW:
      return ErrorSeverity::ERROR;
    case SystemError::SENSOR_INIT_FAILED:
    case SystemError::MEMORY_ALLOCATION_ERROR:
    case SystemError::NOTECARD_INIT_FAILED:
    case SystemError::INVALID_PARAMETER:
      return ErrorSeverity::CRITICAL;
    default:
      return ErrorSeverity::ERROR;
  }
}

void ErrorHandler::logError(SystemError error, const char* context) {
  logError(error, getDefaultSeverity(error), context);
}

void ErrorHandler::logError(SystemError error, ErrorSeverity severity, const char* context) {
  // Store in history
  errorHistory[errorHistoryIndex] = error;
  errorTimestamps[errorHistoryIndex] = millis();
  errorHistoryIndex = (errorHistoryIndex + 1) % MAX_ERROR_HISTORY;
  
  if (errorCount < MAX_ERROR_HISTORY) {
    errorCount++;
  }
  
  // Log to serial
  Serial.print(F("["));
  Serial.print(getSeverityString(severity));
  Serial.print(F("] "));
  Serial.print(getErrorString(error));
  
  if (context != nullptr) {
    Serial.print(F(" ("));
    Serial.print(context);
    Serial.print(F(")"));
  }
  
  Serial.println();
  
  // Additional handling for critical errors
  if (severity == ErrorSeverity::CRITICAL) {
    Serial.println(F("CRITICAL ERROR DETECTED - System may be unstable"));
  }
}

bool ErrorHandler::hasCriticalErrors() const {
  // Check recent errors for critical ones
  for (int i = 0; i < min(errorCount, MAX_ERROR_HISTORY); i++) {
    SystemError error = errorHistory[i];
    if (getDefaultSeverity(error) == ErrorSeverity::CRITICAL) {
      return true;
    }
  }
  return false;
}

SystemError ErrorHandler::getLastError() const {
  if (errorCount == 0) {
    return SystemError::NONE;
  }
  
  int lastIndex = (errorHistoryIndex - 1 + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
  return errorHistory[lastIndex];
}

void ErrorHandler::clearErrors() {
  errorCount = 0;
  errorHistoryIndex = 0;
  Serial.println(F("Error history cleared"));
}

void ErrorHandler::printErrorStats() const {
  Serial.print(F("Error Statistics - Total: "));
  Serial.print(errorCount);
  Serial.print(F(", Critical: "));
  Serial.println(hasCriticalErrors() ? "YES" : "NO");
  
  if (errorCount > 0) {
    Serial.println(F("Recent errors:"));
    int displayCount = min(errorCount, 5); // Show last 5 errors
    for (int i = 0; i < displayCount; i++) {
      int index = (errorHistoryIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
      Serial.print(F("  "));
      Serial.print(getErrorString(errorHistory[index]));
      Serial.print(F(" ("));
      Serial.print(errorTimestamps[index]);
      Serial.println(F("ms)"));
    }
  }
}