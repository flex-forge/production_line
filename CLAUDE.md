# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FlexForge Conveyor Management System - An IoT solution for monitoring production line conveyors using Blues Cygnet STM32L433 MCU and Blues Notecard for cellular connectivity.

## Development Commands

### Arduino IDE
- Board: Blues Cygnet (STM32L433)
- Port: Auto-detect USB
- Upload: Use STLink or USB DFU

### PlatformIO Build Commands
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

## Architecture

### Core Components
1. **Main Loop** (`conveyor_monitor.ino`): Orchestrates sensor reading, data processing, and cloud sync
2. **Sensor Manager**: Hardware abstraction layer for all I2C sensors
3. **Data Processor**: Implements anomaly detection algorithms
4. **Notecard Manager**: Handles cellular IoT communication
5. **Alert Handler**: Manages alert generation and routing

### Key Design Patterns
- Non-blocking sensor reads with configurable intervals
- Circular buffers for vibration analysis
- State-based alert management with deduplication
- Adaptive sync rates based on system state

### Critical Timing
- Encoder ISR: Hardware interrupt for pulse counting
- Vibration sampling: 100Hz for FFT analysis
- Cloud sync: 1 minute normal, immediate for alerts

## Common Tasks

### Adding New Sensors
1. Define I2C address in `config.h`
2. Add initialization in `sensor_manager.cpp::begin()`
3. Implement read function in sensor manager
4. Update `SensorReadings` structure

### Modifying Alert Thresholds
Edit values in `config.h`:
- Speed tolerance: `SPEED_TOLERANCE_PCT`
- Vibration levels: `VIBRATION_WARNING_G`, `VIBRATION_CRITICAL_G`
- Environmental: `TEMP_MIN_C`, `TEMP_MAX_C`, `HUMIDITY_MAX_PCT`

### Debugging Connection Issues
1. Check serial output at 115200 baud
2. Verify I2C addresses with scanner sketch
3. Test Notecard with AT commands
4. Monitor `checkSensorHealth()` output

## Important Considerations

- The rotary encoder uses hardware interrupts on pin 2
- All sensors communicate via I2C at 400kHz
- Notecard requires proper ProductUID configuration
- Gesture sensor needs adequate LED power for reliable detection
- Memory constraints: Avoid dynamic allocation, use fixed buffers