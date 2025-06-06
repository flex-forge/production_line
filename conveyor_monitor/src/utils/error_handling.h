#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>

/**
 * @brief System error codes for consistent error handling
 */
enum class SystemError {
  NONE = 0,
  SENSOR_INIT_FAILED,
  SENSOR_READ_TIMEOUT,
  SENSOR_DATA_INVALID,
  I2C_COMMUNICATION_ERROR,
  MEMORY_ALLOCATION_ERROR,
  NOTECARD_INIT_FAILED,
  NOTECARD_SEND_FAILED,
  CONFIG_VALIDATION_ERROR,
  TELEMETRY_FORMAT_ERROR,
  BUFFER_OVERFLOW,
  INVALID_PARAMETER
};

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
  INFO = 0,
  WARNING = 1,
  ERROR = 2,
  CRITICAL = 3
};

/**
 * @brief Result wrapper for functions that can fail
 */
template<typename T>
struct Result {
  T value;
  SystemError error;
  
  Result(T val) : value(val), error(SystemError::NONE) {}
  Result(SystemError err) : value(T{}), error(err) {}
  
  bool isOk() const { return error == SystemError::NONE; }
  bool isError() const { return error != SystemError::NONE; }
  
  T getValueOr(const T& defaultValue) const {
    return isOk() ? value : defaultValue;
  }
};

/**
 * @brief Simple error handling utility class
 */
class ErrorHandler {
private:
  static const int MAX_ERROR_HISTORY = 10;
  SystemError errorHistory[MAX_ERROR_HISTORY];
  unsigned long errorTimestamps[MAX_ERROR_HISTORY];
  int errorHistoryIndex;
  int errorCount;
  
  const char* getErrorString(SystemError error) const;
  const char* getSeverityString(ErrorSeverity severity) const;

public:
  ErrorHandler();
  
  /**
   * @brief Log an error with automatic severity determination
   * @param error The error code
   * @param context Additional context string
   */
  void logError(SystemError error, const char* context = nullptr);
  
  /**
   * @brief Log an error with explicit severity
   * @param error The error code
   * @param severity Error severity level
   * @param context Additional context string
   */
  void logError(SystemError error, ErrorSeverity severity, const char* context = nullptr);
  
  /**
   * @brief Check if system has critical errors
   * @return true if critical errors exist
   */
  bool hasCriticalErrors() const;
  
  /**
   * @brief Get total error count
   * @return Number of errors logged
   */
  int getErrorCount() const { return errorCount; }
  
  /**
   * @brief Get most recent error
   * @return Most recent error code
   */
  SystemError getLastError() const;
  
  /**
   * @brief Clear error history
   */
  void clearErrors();
  
  /**
   * @brief Print error statistics
   */
  void printErrorStats() const;
};

// Global error handler instance
extern ErrorHandler systemErrorHandler;

// Convenience macros for error logging
#define LOG_ERROR(error) systemErrorHandler.logError(error, __func__)
#define LOG_ERROR_CTX(error, context) systemErrorHandler.logError(error, context)
#define LOG_CRITICAL(error) systemErrorHandler.logError(error, ErrorSeverity::CRITICAL, __func__)

#endif // ERROR_HANDLING_H