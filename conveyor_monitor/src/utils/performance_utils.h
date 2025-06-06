#ifndef PERFORMANCE_UTILS_H
#define PERFORMANCE_UTILS_H

#include <Arduino.h>

/**
 * @brief Performance optimization utilities for embedded systems
 * 
 * This header provides optimized implementations of common operations
 * and memory management utilities specifically designed for microcontrollers.
 */

/**
 * @brief Fast integer square root implementation (faster than sqrt for integers)
 * @param x Input value
 * @return Integer square root of x
 */
inline uint32_t fastSqrt(uint32_t x) {
  if (x == 0) return 0;
  
  uint32_t result = 0;
  uint32_t bit = 1UL << 30; // The second-to-top bit is set
  
  // "bit" starts at the highest power of four <= the argument.
  while (bit > x) {
    bit >>= 2;
  }
  
  while (bit != 0) {
    if (x >= result + bit) {
      x -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }
  return result;
}

/**
 * @brief Fast floating point square root approximation
 * @param x Input value
 * @return Approximate square root (good for ~4-5 decimal places)
 */
inline float fastSqrtf(float x) {
  if (x <= 0.0f) return 0.0f;
  
  // Fast inverse square root (Quake algorithm variant)
  union {
    float f;
    uint32_t i;
  } conv = {x};
  
  conv.i = 0x5f3759df - (conv.i >> 1);
  conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
  
  return x * conv.f; // Multiply by original to get sqrt instead of inverse sqrt
}

/**
 * @brief Stack-based memory pool for temporary allocations
 * @tparam SIZE Size of the memory pool in bytes
 */
template<size_t SIZE>
class StackAllocator {
private:
  uint8_t memory[SIZE];
  size_t offset;
  
public:
  StackAllocator() : offset(0) {}
  
  /**
   * @brief Allocate memory from the stack
   * @param size Number of bytes to allocate
   * @return Pointer to allocated memory, or nullptr if insufficient space
   */
  void* allocate(size_t size) {
    // Align to 4-byte boundary for ARM processors
    size = (size + 3) & ~3;
    
    if (offset + size > SIZE) {
      return nullptr; // Out of memory
    }
    
    void* ptr = &memory[offset];
    offset += size;
    return ptr;
  }
  
  /**
   * @brief Reset allocator (frees all allocations)
   */
  void reset() {
    offset = 0;
  }
  
  /**
   * @brief Get current memory usage
   * @return Number of bytes currently allocated
   */
  size_t getBytesUsed() const {
    return offset;
  }
  
  /**
   * @brief Get available memory
   * @return Number of bytes available for allocation
   */
  size_t getBytesAvailable() const {
    return SIZE - offset;
  }
};

/**
 * @brief Optimized string formatting for telemetry (avoids String class overhead)
 */
class FastStringBuilder {
private:
  char* buffer;
  size_t capacity;
  size_t length;
  
public:
  /**
   * @brief Constructor
   * @param buf Buffer to write to
   * @param cap Capacity of buffer
   */
  FastStringBuilder(char* buf, size_t cap) : buffer(buf), capacity(cap), length(0) {
    if (capacity > 0) {
      buffer[0] = '\0';
    }
  }
  
  /**
   * @brief Append a string literal
   */
  FastStringBuilder& append(const char* str) {
    if (!str) return *this;
    
    while (*str && length < capacity - 1) {
      buffer[length++] = *str++;
    }
    buffer[length] = '\0';
    return *this;
  }
  
  /**
   * @brief Append a floating point number with specified precision
   */
  FastStringBuilder& append(float value, int precision = 2) {
    if (length >= capacity - 1) return *this;
    
    // Handle negative numbers
    if (value < 0) {
      append("-");
      value = -value;
    }
    
    // Integer part
    uint32_t intPart = static_cast<uint32_t>(value);
    appendUInt(intPart);
    
    // Decimal part
    if (precision > 0) {
      append(".");
      
      float fracPart = value - intPart;
      for (int i = 0; i < precision && length < capacity - 1; i++) {
        fracPart *= 10;
        int digit = static_cast<int>(fracPart) % 10;
        buffer[length++] = '0' + digit;
      }
      buffer[length] = '\0';
    }
    
    return *this;
  }
  
  /**
   * @brief Append an unsigned integer
   */
  FastStringBuilder& appendUInt(uint32_t value) {
    if (length >= capacity - 1) return *this;
    
    // Handle zero case
    if (value == 0) {
      buffer[length++] = '0';
      buffer[length] = '\0';
      return *this;
    }
    
    // Find the number of digits
    uint32_t temp = value;
    int digits = 0;
    while (temp > 0) {
      temp /= 10;
      digits++;
    }
    
    // Check if we have space
    if (length + digits >= capacity) {
      return *this;
    }
    
    // Write digits in reverse
    size_t start = length;
    length += digits;
    buffer[length] = '\0';
    
    for (int i = digits - 1; i >= 0; i--) {
      buffer[start + i] = '0' + (value % 10);
      value /= 10;
    }
    
    return *this;
  }
  
  /**
   * @brief Append a boolean value
   */
  FastStringBuilder& append(bool value) {
    return append(value ? "true" : "false");
  }
  
  /**
   * @brief Get current length
   */
  size_t getLength() const {
    return length;
  }
  
  /**
   * @brief Get the built string
   */
  const char* c_str() const {
    return buffer;
  }
  
  /**
   * @brief Reset the builder
   */
  void reset() {
    length = 0;
    if (capacity > 0) {
      buffer[0] = '\0';
    }
  }
};

/**
 * @brief Timing utility for performance measurement
 */
class PerformanceTimer {
private:
  unsigned long startTime;
  unsigned long totalTime;
  uint32_t callCount;
  
public:
  PerformanceTimer() : totalTime(0), callCount(0) {}
  
  /**
   * @brief Start timing
   */
  void start() {
    startTime = micros();
  }
  
  /**
   * @brief Stop timing and accumulate
   */
  void stop() {
    totalTime += (micros() - startTime);
    callCount++;
  }
  
  /**
   * @brief Get average execution time
   * @return Average time in microseconds
   */
  float getAverageTime() const {
    return callCount > 0 ? static_cast<float>(totalTime) / callCount : 0.0f;
  }
  
  /**
   * @brief Get total time
   */
  unsigned long getTotalTime() const {
    return totalTime;
  }
  
  /**
   * @brief Get call count
   */
  uint32_t getCallCount() const {
    return callCount;
  }
  
  /**
   * @brief Reset statistics
   */
  void reset() {
    totalTime = 0;
    callCount = 0;
  }
};

// Global performance timers for key functions
extern PerformanceTimer sensorReadTimer;
extern PerformanceTimer dataProcessTimer;
extern PerformanceTimer telemetryTimer;

// Macro for automatic performance timing
#define PERF_TIME(timer, code) do { \
  timer.start(); \
  code; \
  timer.stop(); \
} while(0)

#endif // PERFORMANCE_UTILS_H