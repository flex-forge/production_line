#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

// System Configuration for FlexForge Conveyor Monitor

// Virtual Sensor Mode - generates fake data if sensor init fails
#define VIRTUAL_SENSOR    1    // Set to 1 to enable virtual sensors

// Timing intervals (milliseconds)
#define SENSOR_READ_INTERVAL     100    // 10Hz for critical sensors
#define DATA_PROCESS_INTERVAL    500    // 2Hz for data processing
#define CLOUD_SYNC_INTERVAL      60000  // 1 minute for normal telemetry
#define HEALTH_CHECK_INTERVAL    30000  // 30 seconds health check

// Notecard configuration
#define NOTECARD_PRODUCT_UID    "com.blues.flex_forge.production_line"
#define NOTECARD_CONTINUOUS     false    // Use periodic sync
#define NOTECARD_SYNC_MINS      5        // Sync every 5 minutes
#define NOTECARD_MOTION_SENSE   true     // Enable motion sensitivity

#endif // SYSTEM_CONFIG_H