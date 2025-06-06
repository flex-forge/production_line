# FlexForge Conveyor Monitor System Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Code Architecture](#code-architecture)
3. [Sensor Operations](#sensor-operations)
4. [Data Processing & Calculations](#data-processing--calculations)
5. [Cloud Telemetry](#cloud-telemetry)
6. [Events & Alerts](#events--alerts)
7. [Performance & Optimization](#performance--optimization)
8. [Manual Testing Guide](#manual-testing-guide)

## System Overview

The FlexForge Conveyor Monitor is an IoT system that monitors production line conveyors using multiple sensors and reports data to the cloud via Blues Notecard cellular connectivity.

### Hardware Components
- **MCU**: Blues Cygnet (STM32L433)
- **Connectivity**: Blues Notecard (cellular IoT)
- **Sensors**: BME688, VL53L1X, LSM9DS1, APDS9960, Adafruit Seesaw Encoder
- **Interface**: I2C bus (400kHz) with Qwiic connectors

### Software Architecture
- **Modular Design**: Organized into specialized functional modules
- **Main Loop**: 10Hz sensor reading, 2Hz data processing, 1/60Hz cloud sync
- **Virtual Sensors**: Fallback mode when physical sensors fail
- **Non-blocking**: All operations designed to avoid delays
- **Performance Optimized**: Circular buffers, fast string building, memory pools
- **Error Resilient**: Comprehensive error handling and recovery systems

## Code Architecture

The codebase is organized into a modular, maintainable structure with clear separation of concerns:

### Directory Structure
```
src/
├── conveyor_monitor.ino    # Main application entry point
├── config/                 # Configuration and data types
│   ├── config.h           # Main configuration aggregator
│   ├── system_config.h    # System timing and Notecard settings
│   ├── sensor_config.h    # I2C addresses and sensor parameters
│   ├── alert_config.h     # Alert and gesture enumerations
│   └── data_types.h       # Core data structures
├── sensors/               # Hardware sensor management
│   ├── sensor_manager.h   # Sensor coordination and I2C management
│   └── sensor_manager.cpp
├── data_processing/       # Analysis algorithms and anomaly detection
│   ├── data_processor.h/.cpp      # Main data processing coordinator
│   ├── anomaly_detector.h/.cpp    # Anomaly detection algorithms
│   └── statistical_analyzer.h/.cpp # Statistical analysis and trends
├── communication/         # External communication systems
│   ├── notecard_manager.h/.cpp    # Cellular IoT via Blues Notecard
│   └── telemetry_formatter.h/.cpp # JSON telemetry formatting
├── alerts/               # Alert management and routing
│   ├── alert_handler.h   # Alert processing and deduplication
│   └── alert_handler.cpp
└── utils/                # Cross-cutting utilities and optimizations
    ├── error_handling.h/.cpp     # Error management system
    ├── circular_buffer.h         # High-performance circular buffer template
    └── performance_utils.h/.cpp  # Performance optimization utilities
```

### Key Architectural Principles

#### **1. Separation of Concerns**
Each module has a single, well-defined responsibility:
- **Sensors**: Hardware abstraction and I2C communication
- **Data Processing**: Statistical analysis and anomaly detection algorithms
- **Communication**: Cloud connectivity and data formatting
- **Alerts**: Alert management, deduplication, and routing
- **Utils**: Cross-cutting concerns like error handling and performance

#### **2. Dependency Flow**
```
Main Application (conveyor_monitor.ino)
    ↓
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   Sensors   │   Data      │ Communication│   Alerts    │
│   Manager   │ Processor   │   Manager    │  Handler    │
└─────────────┴─────────────┴─────────────┴─────────────┘
    ↓               ↓               ↓               ↓
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   Config    │   Utils     │   Config     │   Config    │
│  (sensor)   │ (circular   │  (system)    │  (alerts)   │
│             │  buffers)   │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

#### **3. Error Handling Strategy**
- **Comprehensive Error Classification**: SystemError enum with severity levels
- **Automatic Recovery**: Fallback to virtual sensors when hardware fails
- **Error Tracking**: Historical error logging with periodic reporting
- **Graceful Degradation**: System continues operating with reduced functionality

#### **4. Performance Optimizations**
- **Memory Management**: Circular buffers eliminate dynamic allocation
- **Fast Operations**: Custom string builder avoids Arduino String overhead
- **Real-time Monitoring**: Microsecond-level performance measurement
- **Resource Pooling**: Stack-based memory allocators for temporary data

#### **5. Configuration Management**
- **Modular Configuration**: Split into logical concern-based files
- **Backward Compatibility**: Main config.h aggregates all modules
- **Type Safety**: Separate data types file with comprehensive structures
- **Easy Maintenance**: Clear separation of different configuration aspects

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

The data processing system is built on a modular architecture with specialized components:

### Processing Architecture

#### **DataProcessor (Coordinator)**
- **Central coordination** of all analysis operations
- **Delegates** to specialized analyzer components
- **Maintains** backward-compatible API for existing code
- **Provides** unified interface for anomaly detection and statistics

#### **StatisticalAnalyzer (Analysis Engine)**
- **Circular Buffer Management**: High-performance data history storage
- **Statistical Calculations**: Real-time averages, variance, trends
- **Predictive Analytics**: Maintenance forecasting based on vibration trends
- **Memory Efficient**: Template-based circular buffers eliminate dynamic allocation

#### **AnomalyDetector (Safety Monitor)**
- **Jam Detection**: Vibration-based monitoring for belt anomalies
- **Speed Monitoring**: Deviation detection from operational parameters
- **Environmental Monitoring**: Temperature, humidity, and pressure bounds
- **Threshold Management**: Configurable limits with automatic classification

### Speed Monitoring
```
Encoder Position → Statistical Analysis → Anomaly Detection
├─ Raw Position Data → CircularBuffer<float, 10>
├─ Statistical Calculation → Average, Variance, Trends  
├─ Speed = (Position - Baseline) * 1.0 RPM/detent
├─ Running = Speed > 5.0 RPM
└─ Anomaly = |Speed - 60 RPM| > 6 RPM OR variance > 3.0
```

### Part Counting
```
Distance Reading → Object Detection → Statistical Tracking
├─ ToF Sensor → Distance measurement every 100ms
├─ Object Detection → distance < 100mm threshold
├─ Part Counting → Increment on new detection
├─ Rate Calculation → (count * 60000) / elapsed_time_ms
└─ Historical Tracking → Circular buffer for trend analysis
```

### Vibration Analysis
```
Accelerometer XYZ → Statistical Processing → Anomaly Detection
├─ IMU Data → Magnitude = sqrt(x² + y² + z²)
├─ Circular Buffer → CircularBuffer<float, 30> for 30-sample history
├─ Statistical Analysis → RMS, variance, linear trend calculation
├─ Baseline Establishment → Average of first 30 samples
└─ Anomaly Detection → Compare against thresholds and trends
```

### Advanced Analytics

#### **Anomaly Detection Algorithms**
1. **Speed Anomaly**: Statistical deviation analysis
   - Average speed calculation from CircularBuffer<float, 10>
   - Variance monitoring for stability assessment  
   - Threshold: |Average - 60 RPM| > 6 RPM OR variance > 3.0

2. **Jam Detection**: Physics-based vibration monitoring
   - Continuous vibration level monitoring
   - State tracking: belt running + low vibration = potential jam
   - Threshold: Vibration < 0.3g for > 10 seconds while speed > 5 RPM

3. **Vibration Anomaly**: Multi-factor analysis
   - Real-time RMS calculation from CircularBuffer<float, 30>
   - Linear trend analysis using least-squares regression
   - Threshold: RMS > 1.0g (warning) or > 2.0g (critical) or positive trend > 0.01g/sample

4. **Environmental Anomaly**: Boundary and rate monitoring
   - Temperature: < 10°C or > 40°C (boundary) + variance > 5°C (rate)
   - Humidity: > 80% with trend analysis
   - Pressure: Deviation monitoring with statistical analysis

#### **Predictive Maintenance**
- **Vibration Trend Analysis**: Linear regression on 30-sample history
- **Time-to-Failure Estimation**: Based on current level vs critical threshold
- **Maintenance Scheduling**: Hours until intervention needed
- **Efficiency Scoring**: Weighted combination of speed (40%) + vibration (40%) + jam penalty (20%)

## Cloud Telemetry

The telemetry system uses optimized formatting and validation for efficient cellular transmission.

### Optimized Telemetry Processing

#### **TelemetryFormatter Architecture**
- **FastStringBuilder**: Stack-based JSON generation (40-60% faster than String)
- **Data Validation**: Comprehensive input sanitization and error detection
- **Memory Efficient**: Fixed-buffer allocation, no heap fragmentation
- **Error Recovery**: Graceful handling of invalid sensor data with fallback values

#### **Performance Characteristics**
- **Formatting Time**: ~50-200μs (vs 300-500μs with Arduino String)
- **Memory Usage**: Stack-allocated 512-byte buffer
- **Validation**: NaN/infinite value detection and substitution
- **Error Handling**: Automatic logging with SystemError classification

### Telemetry Payload (every 60 seconds)
```json
{
  "speed_rpm": 15.5,
  "parts_per_min": 30,
  "vibration": 0.45,
  "temp": 22.5,
  "humidity": 45.0,
  "pressure": 1013.2,
  "gas_resistance": 150000,
  "running": true,
  "operator": true
}
```

### Optimized Data Flow
```
Sensors (100ms) → SystemState (500ms) → Telemetry Processing (60s)
     ↓                    ↓                        ↓
SensorReadings    →   Validation    →    FastStringBuilder
     ↓                    ↓                        ↓
Circular Buffers  →   Error Check   →    JSON Generation  
     ↓                    ↓                        ↓
Statistical       →   Data Sanity   →    Notecard Transmission
Analysis              (fallbacks)          (cellular optimized)
```

### Enhanced Data Pipeline
1. **Sensor Reading** (100ms intervals): Hardware abstraction with I2C optimization
2. **Statistical Processing** (500ms intervals): Circular buffer analytics and anomaly detection  
3. **Telemetry Generation** (60s intervals): Optimized JSON formatting with validation
4. **Cloud Transmission**: Blues Notecard cellular with automatic retry and error handling

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

## Performance & Optimization

The system includes comprehensive performance monitoring and optimization features designed for embedded systems.

### Performance Monitoring

#### **Real-time Performance Measurement**
The system automatically tracks execution time for critical operations:

- **Sensor Reading**: Time to read all I2C sensors (typically 2-5ms)
- **Data Processing**: Statistical analysis and anomaly detection (typically 100-500μs)
- **Telemetry Formatting**: JSON generation and validation (typically 50-200μs)

#### **Performance Statistics Reporting**
Every 5 minutes, the system logs performance statistics:
```
=== Performance Statistics ===
Sensor Read - Avg: 3.2ms, Calls: 3000
Data Process - Avg: 0.15ms, Calls: 600  
Telemetry - Avg: 0.08ms, Calls: 5
```

### Memory Optimizations

#### **Circular Buffer Implementation**
- **Zero Dynamic Allocation**: Fixed-size buffers eliminate heap fragmentation
- **Automatic Management**: Oldest data automatically overwritten when full
- **Built-in Analytics**: Average, variance, min/max calculations without loops
- **Template-based**: `CircularBuffer<float, 30>` for type safety

#### **Fast String Operations**
- **FastStringBuilder**: Eliminates Arduino String allocations (40-60% faster)
- **Stack-based**: Uses provided buffer, no heap allocation
- **Optimized Formatting**: Custom float-to-string for embedded systems
- **Reduced Memory**: Predictable memory usage patterns

#### **Memory Pool Management**
- **StackAllocator**: Temporary allocations from fixed memory pool
- **4-byte Alignment**: Optimized for ARM processor performance
- **Reset Capability**: Quick cleanup of all temporary allocations

### Performance Characteristics

#### **Execution Time Targets**
- **Sensor Reading Loop**: <10ms (100Hz capable)
- **Data Processing**: <1ms (1kHz capable) 
- **Telemetry Generation**: <500μs (2kHz capable)
- **Total System Cycle**: <15ms average

#### **Memory Usage**
- **Static Allocation**: Predictable memory usage (~8KB RAM)
- **No Heap Fragmentation**: All allocations stack or static
- **Circular Buffers**: Fixed 2.8KB for all historical data
- **Error Tracking**: 240 bytes for error history

#### **Power Efficiency**
- **Sleep-friendly**: No blocking operations in main loop
- **I2C Optimization**: Batched sensor reads reduce bus overhead
- **Cellular Efficiency**: Optimized JSON reduces transmission time

### Optimization Guidelines

#### **Adding New Features**
1. **Use Circular Buffers**: For any historical data (see `utils/circular_buffer.h`)
2. **Avoid Dynamic Allocation**: Use stack allocators or static buffers
3. **Profile Performance**: Use `PERF_TIME(timer, code)` macro for measurement
4. **Error Handling**: Always use `SystemError` enum for consistent reporting

#### **Performance Debugging**
1. **Enable Performance Logging**: Statistics reported every 5 minutes
2. **Monitor Individual Operations**: Use PerformanceTimer class
3. **Check Memory Usage**: StackAllocator provides usage reporting
4. **I2C Bus Analysis**: Monitor sensor read times for communication issues

#### **Best Practices**
- **Prefer Templates**: CircularBuffer over fixed arrays
- **Use FastStringBuilder**: For any string construction
- **Stack Allocation**: For temporary data structures
- **Const Correctness**: All getters marked const to prevent accidental modifications

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