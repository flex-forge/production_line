# FlexForge Conveyor Management System

## Overview

The FlexForge Conveyor Management System is an IoT solution designed to monitor production line conveyors in real-time, preventing costly downtime through predictive maintenance and instant alerts. Built for the Blues Cygnet STM32L433 MCU with Blues Notecard connectivity.

## Key Features

- **Real-Time Monitoring**: Speed, vibration, environmental conditions, and part detection
- **Predictive Maintenance**: Advanced analytics and anomaly detection algorithms
- **Instant Alerts**: Critical notifications for jams, anomalies, and sensor failures
- **Operator Interface**: Gesture-based controls for system interaction
- **Cloud Connectivity**: Cellular IoT via Blues Notecard with optimized telemetry
- **Performance Optimized**: Circular buffers, fast string operations, and memory-efficient design

## Hardware Requirements

### Core Components
- **MCU**: Blues Cygnet STM32L433
- **Connectivity**: Blues Notecard (cellular IoT)
- **Interface**: I2C bus (400kHz) with Qwiic connectors

### Sensors
- **Adafruit Seesaw Encoder**: Conveyor speed control/simulation (I2C 0x36)
- **VL53L1X ToF Distance Sensor**: Part detection (I2C 0x29) 
- **LSM9DS1 9-DOF IMU**: Vibration analysis (I2C 0x6B/0x1E)
- **BME688 Environmental Sensor**: Temperature, humidity, pressure, air quality (I2C 0x76)
- **APDS9960 Gesture Sensor**: Operator interface (I2C 0x39)

## Build & Development

### PlatformIO (Recommended)
```bash
# Build firmware
pio run -e blues_cygnet

# Upload firmware  
pio run -e blues_cygnet -t upload

# Serial monitor
pio device monitor -b 115200

# Clean build
pio run -e blues_cygnet -t clean
```

### Arduino IDE
- **Board**: Blues Cygnet (STM32L433)
- **Port**: Auto-detect USB
- **Upload**: Use STLink or USB DFU

### Required Libraries
- Notecard (Blues Wireless)
- Adafruit BME680
- VL53L1X (Pololu) 
- SparkFun LSM9DS1
- SparkFun APDS9960

## Quick Start

1. **Hardware Setup**: Connect all sensors via I2C/Qwiic
2. **Notecard Config**: Set your ProductUID in system configuration
3. **Build & Upload**: Use PlatformIO or Arduino IDE
4. **Monitor**: Connect serial at 115200 baud for real-time status

## Configuration

System configuration is modular and organized in `src/config/`:
- **System settings**: Timing intervals, Notecard parameters
- **Sensor parameters**: I2C addresses, sampling rates, thresholds
- **Alert configuration**: Gesture mappings, alert levels

## Operation

### Operator Controls (Gesture Interface)
- **Swipe Up**: Acknowledge jam clearance (30-second window)
- **Swipe Left**: Resume system monitoring
- **Swipe Right**: Pause system monitoring

### System Status
Monitor via serial output at 115200 baud for:
- Sensor readings every 10 seconds
- Cloud sync events every 60 seconds  
- Real-time alerts and operator actions
- Performance statistics every 5 minutes

## Technical Documentation

For detailed technical information, see **[SYSTEM_DOCUMENTATION.md](./SYSTEM_DOCUMENTATION.md)**:

- **Code Architecture**: Directory structure, dependency flow, design patterns
- **Sensor Operations**: Detailed sensor configurations and data processing
- **Data Processing**: Statistical analysis, anomaly detection algorithms
- **Performance Optimization**: Circular buffers, fast operations, memory management
- **Testing Guide**: Comprehensive manual testing procedures
- **Troubleshooting**: Common issues and debugging strategies

## Business Value

- **40% Reduction in Downtime**: Predictive alerts before failures
- **75% Faster Response**: Instant mobile notifications  
- **Improved Maintenance**: Data-driven service schedules
- **Multi-Site Visibility**: Central monitoring dashboard
- **Compliance Tracking**: Complete audit trail

## License

Copyright (c) 2024 FlexForge Industries
Licensed under MIT License