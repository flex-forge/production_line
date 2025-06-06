# FlexForge Conveyor Monitor System Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Sensor Operations](#sensor-operations)
3. [Data Processing & Calculations](#data-processing--calculations)
4. [Cloud Telemetry](#cloud-telemetry)
5. [Events & Alerts](#events--alerts)
6. [Manual Testing Guide](#manual-testing-guide)

## System Overview

The FlexForge Conveyor Monitor is an IoT system that monitors production line conveyors using multiple sensors and reports data to the cloud via Blues Notecard cellular connectivity.

### Hardware Components
- **MCU**: Blues Cygnet (STM32L433)
- **Connectivity**: Blues Notecard (cellular IoT)
- **Sensors**: BME688, VL53L1X, LSM9DS1, APDS9960, Adafruit Seesaw Encoder
- **Interface**: I2C bus (400kHz) with Qwiic connectors

### Software Architecture
- **Main Loop**: 10Hz sensor reading, 2Hz data processing, 1/60Hz cloud sync
- **Virtual Sensors**: Fallback mode when physical sensors fail
- **Non-blocking**: All operations designed to avoid delays

## Sensor Operations

### 1. Rotary Encoder (Adafruit Seesaw - I2C 0x36)
**Purpose**: Speed control dial for conveyor belt

**Operation**:
- Reads absolute position at startup as "zero speed" baseline
- Speed calculated as: `speed_rpm = (current_position - baseline_position) * 1.0`
- Range: 0-100 RPM (clamped)
- Resolution: 24 detents per revolution

**Data Output**:
- `speed_rpm`: Current conveyor speed in RPM
- `conveyorRunning`: true if speed > 5 RPM

### 2. Environmental Sensor (SparkFun BME688 - I2C 0x76)
**Purpose**: Monitor ambient conditions

**Operation**:
- Reads every 100ms
- Oversampling: Temperature 8x, Humidity 2x, Pressure 4x
- Gas sensor heated to 320°C for 150ms

**Data Output**:
- `temperature`: Celsius (°C)
- `humidity`: Relative humidity (%)
- `pressure`: Atmospheric pressure (hPa)
- `gasResistance`: Air quality indicator (Ohms)

### 3. Time-of-Flight Distance Sensor (VL53L1X - I2C 0x29)
**Purpose**: Detect parts passing on conveyor

**Operation**:
- Short range mode for close detection
- 50ms timing budget, 100ms continuous interval
- Detection threshold: < 100mm indicates part present

**Data Output**:
- `distance_mm`: Distance to nearest object
- `objectDetected`: true if distance < 100mm
- `partCount`: Running count of detected parts

### 4. IMU (SparkFun LSM9DS1 - I2C 0x6B/0x1E)
**Purpose**: Vibration analysis for anomaly detection

**Operation**:
- Accelerometer: ±8g range, 119Hz sampling
- Gyroscope: ±245 dps, 119Hz sampling
- Magnetometer: Earth field measurement

**Data Output**:
- `vibrationMagnitude`: RMS acceleration magnitude (g)
- Raw acceleration, gyro, and magnetometer data

### 5. Gesture Sensor (SparkFun APDS9960 - I2C 0x39)
**Purpose**: Operator presence and gesture control

**Operation**:
- Gesture detection: swipe up/down/left/right, wave
- Proximity: 0-255 scale
- 2-second cooldown between gestures
- LED drive: 25mA, Gesture gain: 2x

**Data Output**:
- `operatorPresent`: true if proximity > 10
- `lastGesture`: Most recent gesture type

**Gesture Actions**:
- **Swipe Up**: Acknowledge/clear jam alerts (30-second window)
- **Swipe Left**: Resume system monitoring
- **Swipe Right**: Pause system monitoring
- **Swipe Down**: Reserved (no current action)
- **Wave**: Reserved (no current action)

## Data Processing & Calculations

### Speed Monitoring
```
Encoder Position → Speed Calculation → Running State
- Speed = (Position - Baseline) * 1.0 RPM/detent
- Running = Speed > 5.0 RPM
```

### Part Counting
```
Distance Reading → Object Detection → Part Counter
- If distance < 100mm AND was_not_detected_before:
  - Increment part count
  - Update last detection time
- Parts per minute = (count * 60000) / elapsed_time_ms
```

### Vibration Analysis
```
Accelerometer XYZ → Magnitude → RMS Calculation
- Magnitude = sqrt(x² + y² + z²)
- Store in 256-sample circular buffer
- RMS = sqrt(sum(magnitudes²) / 256)
```

### Anomaly Detection
1. **Speed Anomaly**: Speed deviation > 10% from nominal (60 RPM)
2. **Jam Detection**: Vibration < 0.3g for > 10 seconds while belt should be running (speed > 5 RPM)
3. **Vibration Anomaly**: RMS > 1.0g (warning) or > 2.0g (critical)
4. **Environmental Anomaly**: Temp < 10°C or > 40°C, Humidity > 80%

## Cloud Telemetry

### Telemetry Payload (every 60 seconds)
```json
{
  "speed_rpm": 15.5,
  "parts_per_min": 30,
  "vibration": 0.45,
  "temp": 22.5,
  "humidity": 45.0,
  "running": true,
  "operator": true
}
```

### Data Flow
1. Sensors read every 100ms → `SensorReadings` structure
2. Processed every 500ms → `SystemState` structure
3. Validated and formatted → JSON string
4. Sent via Notecard → `telemetry.qo` file

## Operator Interactions

### Presence Detection
- **Method**: APDS9960 proximity sensor (>10 units = present)
- **Update Rate**: Every 100ms sensor reading
- **Telemetry**: Included in regular cloud sync as `"operator":true/false`

### Gesture Control System

#### Supported Gestures
| Gesture | Action | Description |
|---------|--------|-------------|
| **Swipe Up** | Jam Acknowledgment | Clear jam alerts within 30-second window |
| **Swipe Left** | Resume Monitoring | Resume system monitoring after pause |
| **Swipe Right** | Pause Monitoring | Temporarily pause system monitoring |
| **Swipe Down** | Reserved | No current action assigned |
| **Wave** | Reserved | No current action assigned |

#### Gesture Processing
- **Cooldown**: 2-second minimum between gestures (prevents spamming the sensor)
- **Detection**: Requires clear gesture pattern from APDS9960
- **Validation**: Automatic gesture clearing after processing
- **Response**: Immediate cloud event transmission

### Operator Event Types

#### Cloud Events Generated
1. **`operator.action`** events:
   - `jam_cleared`: Operator acknowledged jam via swipe up
   - `monitoring_paused`: Operator paused monitoring via swipe right
   - `monitoring_resumed`: Operator resumed monitoring via swipe left

2. **`alert.acknowledged`** events:
   - Automatic when any alert is acknowledged
   - Includes alert type and acknowledgment method

#### Alert Acknowledgment System
- **Jam Alerts**: 30-second acknowledgment window after detection
- **Auto-Clear**: Jam alerts clear automatically if conveyor resumes normal operation
- **Manual Clear**: Swipe up gesture within acknowledgment window
- **Effect**: Removes alert from pending queue, sends acknowledgment event

### Operator Workflow Examples

#### Jam Response Workflow
1. **Jam Detected**: System detects low vibration while running
2. **Alert Generated**: Critical jam alert sent to cloud immediately
3. **Operator Response**: Operator investigates and fixes jam
4. **Acknowledgment**: Operator swipes up within 30 seconds
5. **Resolution**: Alert acknowledged, `jam_cleared` event sent

#### Maintenance Workflow  
1. **Maintenance Start**: Operator swipes right to pause monitoring
2. **Monitoring Paused**: `monitoring_paused` event sent to cloud
3. **Maintenance Work**: System continues telemetry but suppresses alerts
4. **Maintenance Complete**: Operator swipes left to resume monitoring
5. **Monitoring Resumed**: `monitoring_resumed` event sent to cloud

## Events & Alerts

### Event Types
1. **System Events**
   - `system.startup`: Boot with sensor status
   - `system.shutdown`: Graceful shutdown

2. **Operator Events**
   - `operator.action`: Gesture-based actions (jam_cleared, monitoring_paused, monitoring_resumed)
   - `alert.acknowledged`: Manual alert acknowledgments

3. **Alert Events**
   - `speed_anomaly`: Speed deviation detected
   - `jam_detected`: Conveyor jam
   - `vibration_high`: Excessive vibration
   - `env_condition`: Environmental threshold exceeded
   - `sensor_failure`: Sensor communication error

### Alert Levels
- **INFO (0)**: Informational only
- **WARNING (1)**: Attention needed
- **CRITICAL (2)**: Immediate action required

### Alert Deduplication
- Same alert type suppressed for 5 minutes
- Critical alerts always sent immediately
- Alerts sync with `urgent:true` flag

## Manual Testing Guide

### 1. Test Encoder Speed Control
```
1. Power on system - note "baseline position" in serial output
2. Turn encoder clockwise - speed should increase (1 RPM per detent)
3. Turn encoder counterclockwise - speed should decrease
4. Verify speed shows in "=== Sensor Readings ===" output
5. Speed > 5 RPM should show "Running: 1"
```

### 2. Test Part Detection
```
1. Place object < 100mm from VL53L1X sensor
2. Remove object
3. Repeat several times
4. Check "Parts/min" increases in sensor readings
5. Verify part count in telemetry payload
```

### 3. Test Vibration Detection
```
1. Tap or shake the IMU sensor
2. Watch "Vibration" value in sensor readings
3. Strong shaking > 1.0g should trigger warning alert
4. Very strong shaking > 2.0g should trigger critical alert
```

### 4. Test Environmental Monitoring
```
1. Breathe on BME688 sensor - humidity should increase
2. Touch sensor - temperature should increase slightly
3. Check values update in sensor readings
4. Extreme values trigger environmental alerts
```

### 5. Test Operator Detection and Gestures
```
1. Wave hand near APDS9960 sensor
2. "Operator: 1" should appear in readings
3. Move hand away - "Operator: 0" after timeout

Gesture Testing:
4. Swipe UP (palm toward sensor, move up) - should see jam acknowledgment event
5. Swipe RIGHT (palm toward sensor, move right) - should see monitoring paused event  
6. Swipe LEFT (palm toward sensor, move left) - should see monitoring resumed event
7. Swipe DOWN/WAVE - should detect gesture but no action
8. Wait 2 seconds between gestures (cooldown period)
9. Check for operator.action events in cloud output
```

### 6. Test Cloud Connectivity
```
1. Wait for "=== Cloud Sync Triggered ===" message
2. Verify "Telemetry JSON:" shows complete data
3. Check Notecard response shows successful add
4. Telemetry sent every 60 seconds
5. Alerts sent immediately with sync:true
```

### 7. Test Jam Detection and Acknowledgment
```
1. Set encoder to >5 RPM (belt should be running)
2. Keep IMU very still to simulate stopped belt
3. After 10 seconds, should see jam detection messages
4. Within 30 seconds, swipe UP near APDS9960 sensor
5. Should see "jam_cleared" operator.action event
6. OR move IMU to generate vibration >0.3g for auto-clear
```

### 8. Test Virtual Sensors
```
1. Disconnect a sensor physically
2. Reboot system
3. Check for "Using virtual X data" messages
4. Verify system continues operating
5. Virtual data should show realistic variations
```

### Serial Monitor Commands
Connect at 115200 baud to see:
- Sensor initialization status
- Sensor readings every 10 seconds
- Encoder debug every 5 seconds
- Cloud sync events every 60 seconds
- All alerts and events in real-time

### Common Issues
1. **No speed reading**: Check encoder baseline was set at desired zero position
2. **No part detection**: Ensure objects pass within 100mm of ToF sensor
3. **Missing telemetry values**: Usually indicates sensor initialization failure
4. **High vibration readings**: Check IMU mounting, should be firmly attached
5. **No operator detection**: APDS9960 needs adequate LED power, check connections
6. **Gestures not working**: 
   - Ensure 2-second cooldown between gestures
   - Hand should be 2-10cm from sensor for optimal detection
   - Check for "APDS9960 init failed" message at startup
7. **Jam alerts not clearing**: 
   - Swipe up must be within 30 seconds of jam detection
   - Check operator presence is detected first (proximity > 10)
8. **Operator events not in cloud**: Check immediate sync after gesture detection