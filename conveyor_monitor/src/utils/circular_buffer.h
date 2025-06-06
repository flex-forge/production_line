#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <Arduino.h>

/**
 * @brief Template class for efficient circular buffer implementation
 * 
 * This template provides a fixed-size circular buffer that is memory efficient
 * and avoids dynamic allocation. Perfect for embedded systems where memory
 * usage must be predictable and allocation-free.
 * 
 * @tparam T Data type to store in the buffer
 * @tparam SIZE Maximum number of elements in the buffer (must be compile-time constant)
 */
template<typename T, size_t SIZE>
class CircularBuffer {
private:
  T buffer[SIZE];           ///< Internal storage array
  size_t head;              ///< Index of next write position
  size_t tail;              ///< Index of oldest element  
  size_t count;             ///< Current number of elements
  bool overwrite;           ///< Whether to overwrite when full

public:
  /**
   * @brief Constructor - initializes empty buffer
   * @param allowOverwrite If true, new data overwrites old when full (default: true)
   */
  explicit CircularBuffer(bool allowOverwrite = true) 
    : head(0), tail(0), count(0), overwrite(allowOverwrite) {
    // Initialize buffer with default values
    for (size_t i = 0; i < SIZE; i++) {
      buffer[i] = T{};
    }
  }
  
  /**
   * @brief Add an element to the buffer
   * @param item Element to add
   * @return true if added successfully, false if buffer full and overwrite disabled
   */
  bool push(const T& item) {
    if (isFull() && !overwrite) {
      return false; // Buffer full and overwrite not allowed
    }
    
    buffer[head] = item;
    head = (head + 1) % SIZE;
    
    if (isFull()) {
      // Overwriting oldest element, move tail forward
      tail = (tail + 1) % SIZE;
    } else {
      count++;
    }
    
    return true;
  }
  
  /**
   * @brief Remove and return the oldest element
   * @param item Reference to store the removed element
   * @return true if element removed, false if buffer empty
   */
  bool pop(T& item) {
    if (isEmpty()) {
      return false;
    }
    
    item = buffer[tail];
    tail = (tail + 1) % SIZE;
    count--;
    return true;
  }
  
  /**
   * @brief Get element at specific index (0 = oldest)
   * @param index Index of element to retrieve (0-based from oldest)
   * @return Reference to element at index
   * @throws None (unchecked access for performance)
   */
  const T& operator[](size_t index) const {
    return buffer[(tail + index) % SIZE];
  }
  
  /**
   * @brief Get element at specific index (0 = oldest) - mutable version
   * @param index Index of element to retrieve
   * @return Reference to element at index
   */
  T& operator[](size_t index) {
    return buffer[(tail + index) % SIZE];
  }
  
  /**
   * @brief Get the newest (most recently added) element
   * @return Reference to newest element
   * @throws None (undefined behavior if buffer empty)
   */
  const T& newest() const {
    size_t newest_index = (head - 1 + SIZE) % SIZE;
    return buffer[newest_index];
  }
  
  /**
   * @brief Get the oldest element without removing it
   * @return Reference to oldest element
   * @throws None (undefined behavior if buffer empty)
   */
  const T& oldest() const {
    return buffer[tail];
  }
  
  /**
   * @brief Check if buffer is empty
   * @return true if no elements in buffer
   */
  bool isEmpty() const {
    return count == 0;
  }
  
  /**
   * @brief Check if buffer is full
   * @return true if buffer contains maximum elements
   */
  bool isFull() const {
    return count == SIZE;
  }
  
  /**
   * @brief Get current number of elements
   * @return Number of elements currently in buffer
   */
  size_t size() const {
    return count;
  }
  
  /**
   * @brief Get maximum capacity of buffer
   * @return Maximum number of elements buffer can hold
   */
  size_t capacity() const {
    return SIZE;
  }
  
  /**
   * @brief Clear all elements from buffer
   */
  void clear() {
    head = 0;
    tail = 0;
    count = 0;
  }
  
  /**
   * @brief Calculate average of all elements (for numeric types)
   * @return Average value of all elements in buffer
   * @note Only valid for numeric types that support +, /, and default construction
   */
  T average() const {
    if (isEmpty()) {
      return T{};
    }
    
    T sum = T{};
    for (size_t i = 0; i < count; i++) {
      sum += (*this)[i];
    }
    
    return sum / static_cast<T>(count);
  }
  
  /**
   * @brief Calculate variance of all elements (for numeric types)
   * @param mean Pre-calculated mean value (optional optimization)
   * @return Variance of all elements in buffer
   */
  T variance(const T& mean) const {
    if (count <= 1) {
      return T{};
    }
    
    T sumSquares = T{};
    for (size_t i = 0; i < count; i++) {
      T diff = (*this)[i] - mean;
      sumSquares += diff * diff;
    }
    
    return sumSquares / static_cast<T>(count);
  }
  
  /**
   * @brief Find minimum value in buffer (for comparable types)
   * @return Minimum value found
   */
  T min() const {
    if (isEmpty()) {
      return T{};
    }
    
    T minimum = (*this)[0];
    for (size_t i = 1; i < count; i++) {
      if ((*this)[i] < minimum) {
        minimum = (*this)[i];
      }
    }
    return minimum;
  }
  
  /**
   * @brief Find maximum value in buffer (for comparable types)
   * @return Maximum value found
   */
  T max() const {
    if (isEmpty()) {
      return T{};
    }
    
    T maximum = (*this)[0];
    for (size_t i = 1; i < count; i++) {
      if ((*this)[i] > maximum) {
        maximum = (*this)[i];
      }
    }
    return maximum;
  }
  
  /**
   * @brief Iterator support for range-based for loops
   */
  class iterator {
  private:
    const CircularBuffer* buffer;
    size_t index;
    
  public:
    iterator(const CircularBuffer* buf, size_t idx) : buffer(buf), index(idx) {}
    
    const T& operator*() const { return (*buffer)[index]; }
    iterator& operator++() { ++index; return *this; }
    bool operator!=(const iterator& other) const { return index != other.index; }
  };
  
  /**
   * @brief Begin iterator for range-based loops
   */
  iterator begin() const { return iterator(this, 0); }
  
  /**
   * @brief End iterator for range-based loops
   */
  iterator end() const { return iterator(this, count); }
};

// Type aliases for common use cases
using FloatBuffer8 = CircularBuffer<float, 8>;
using FloatBuffer16 = CircularBuffer<float, 16>;
using FloatBuffer32 = CircularBuffer<float, 32>;
using IntBuffer8 = CircularBuffer<int, 8>;
using IntBuffer16 = CircularBuffer<int, 16>;

#endif // CIRCULAR_BUFFER_H