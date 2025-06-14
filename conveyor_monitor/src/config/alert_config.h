#ifndef ALERT_CONFIG_H
#define ALERT_CONFIG_H

// Alert levels
enum AlertLevel {
  ALERT_INFO = 0,
  ALERT_WARNING = 1,
  ALERT_CRITICAL = 2
};

// Alert types
enum AlertType {
  ALERT_NONE = 0,
  ALERT_SPEED_ANOMALY,
  ALERT_JAM_DETECTED,
  ALERT_VIBRATION_HIGH,
  ALERT_ENV_CONDITION,
  ALERT_SENSOR_FAILURE,
  ALERT_COMM_FAILURE
};

// Gesture types
enum GestureType {
  GESTURE_NONE = 0,
  GESTURE_SWIPE_UP,
  GESTURE_SWIPE_DOWN,
  GESTURE_SWIPE_LEFT,
  GESTURE_SWIPE_RIGHT,
  GESTURE_WAVE
};

#endif // ALERT_CONFIG_H