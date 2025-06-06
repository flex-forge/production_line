#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

// Sensor I2C addresses
#define BME688_I2C_ADDR         0x76    // Environmental sensor
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
#define JAM_DETECT_TIME_MS       10000   // Time with low vibration = jam (10 seconds)
#define JAM_VIBRATION_THRESHOLD  0.3     // Vibration level below which indicates jam (g)
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

#endif // SENSOR_CONFIG_H