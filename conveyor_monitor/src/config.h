#ifndef CONFIG_H
#define CONFIG_H

// System Configuration for FlexForge Conveyor Monitor

// Virtual Sensor Mode - generates fake data if sensor init fails
#define VIRTUAL_SENSOR    1    // Set to 1 to enable virtual sensors

// Timing intervals (milliseconds)
#define SENSOR_READ_INTERVAL     100    // 10Hz for critical sensors
#define DATA_PROCESS_INTERVAL    500    // 2Hz for data processing
#define CLOUD_SYNC_INTERVAL      60000  // 1 minute for normal telemetry
#define HEALTH_CHECK_INTERVAL    30000  // 30 seconds health check

// Sensor I2C addresses
#define BME688_I2C_ADDR         0x77    // Environmental sensor
#define VL53L1X_I2C_ADDR        0x29    // ToF distance sensor
#define LSM9DS1_AG_I2C_ADDR     0x6B    // Accel/Gyro
#define LSM9DS1_M_I2C_ADDR      0x1E    // Magnetometer
#define APDS9960_I2C_ADDR       0x39    // Gesture sensor
#define SEESAW_I2C_ADDR         0x36    // Rotary encoder (Adafruit Seesaw)

// Conveyor parameters
#define SEESAW_ENCODER_MODULE    1       // Seesaw encoder module number
#define ENCODER_PULSES_PER_REV   24      // Seesaw encoder resolution (24 detents)
#define CONVEYOR_GEAR_RATIO      5.0     // Motor to belt ratio
#define NOMINAL_SPEED_RPM        60.0    // Expected conveyor speed
#define MIN_SPEED_THRESHOLD      5.0     // Below this = stopped
#define SPEED_TOLERANCE_PCT      10.0    // Acceptable speed variation %

// Part detection parameters
#define PART_DISTANCE_MM         50      // Expected distance to parts
#define PART_DETECT_THRESHOLD    100     // Detection distance threshold
#define JAM_DETECT_TIME_MS       5000    // Time without movement = jam
#define EXPECTED_PARTS_PER_MIN   30      // Normal production rate

// Vibration analysis parameters
#define VIBRATION_SAMPLE_RATE    100     // Hz
#define VIBRATION_SAMPLE_SIZE    256     // Samples for FFT
#define VIBRATION_BASELINE_G     0.5     // Normal vibration level
#define VIBRATION_WARNING_G      1.0     // Warning threshold
#define VIBRATION_CRITICAL_G     2.0     // Critical threshold

// Environmental thresholds
#define TEMP_MIN_C              10.0     // Minimum operating temp
#define TEMP_MAX_C              40.0     // Maximum operating temp
#define TEMP_WARNING_C          35.0     // Warning threshold
#define HUMIDITY_MAX_PCT        80.0     // Maximum humidity
#define AIR_QUALITY_THRESHOLD   250      // IAQ threshold

// Operator interaction
#define JAM_ACK_WINDOW          30000    // 30s to acknowledge jam
#define GESTURE_COOLDOWN_MS     2000     // Prevent gesture spam

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

// System state structure
struct SystemState {
  bool conveyorRunning;
  float speed_rpm;
  int partsPerMinute;
  float vibrationLevel;
  float temperature;
  float humidity;
  unsigned long lastJamTime;
  bool operatorPresent;
};

// Sensor reading structure
struct SensorReadings {
  // Encoder
  float encoderSpeed;
  int32_t encoderPulses;

  // ToF
  uint16_t distance_mm;
  bool objectDetected;

  // IMU
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float mag_x, mag_y, mag_z;

  // Environmental
  float temperature;
  float humidity;
  float pressure;
  uint32_t gasResistance;

  // Gesture
  uint8_t gesture;
  uint8_t proximity;
};

// Notecard configuration
#define NOTECARD_PRODUCT_UID    "com.blues.flex_forge.production_line"
#define NOTECARD_CONTINUOUS     false    // Use periodic sync
#define NOTECARD_SYNC_MINS      5        // Sync every 5 minutes
#define NOTECARD_MOTION_SENSE   true     // Enable motion sensitivity

#endif // CONFIG_H