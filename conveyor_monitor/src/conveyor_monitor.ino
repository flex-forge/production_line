/**
 * FlexForge Conveyor Management System
 * 
 * Real-time monitoring system for production line conveyors
 * Target: Blues Cygnet STM32L433 MCU with Blues Notecard
 * 
 * Features:
 * - Speed monitoring via rotary encoder
 * - Jam detection using ToF sensor
 * - Predictive maintenance through vibration analysis
 * - Environmental condition monitoring
 * - Operator gesture interface
 */

#include <Wire.h>
#include <Notecard.h>
#include "config.h"
#include "sensor_manager.h"
#include "data_processor.h"
#include "notecard_manager.h"
#include "alert_handler.h"

// Global objects
SensorManager sensorManager;
DataProcessor dataProcessor;
NotecardManager notecardManager;
AlertHandler alertHandler;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastDataProcess = 0;
unsigned long lastCloudSync = 0;
unsigned long lastHealthCheck = 0;

// System state
SystemState currentState = {
  .conveyorRunning = false,
  .speed_rpm = 0.0,
  .partsPerMinute = 0,
  .vibrationLevel = 0.0,
  .temperature = 0.0,
  .humidity = 0.0,
  .lastJamTime = 0,
  .operatorPresent = false
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // Wait up to 5 seconds for serial
  
  Serial.println(F("FlexForge Conveyor Monitor v1.0"));
  Serial.println(F("Initializing..."));
  
  // Initialize I2C bus
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C
  
  // Initialize components
  if (!sensorManager.begin()) {
    Serial.println(F("ERROR: Sensor initialization failed!"));
    while(1) { delay(1000); } // Halt
  }
  
  if (!notecardManager.begin()) {
    Serial.println(F("ERROR: Notecard initialization failed!"));
    while(1) { delay(1000); } // Halt
  }
  
  dataProcessor.begin();
  alertHandler.begin(&notecardManager);
  
  Serial.println(F("System ready!"));
  
  // Send startup notification
  notecardManager.sendEvent("system.startup", "{\"version\":\"1.0\",\"sensors\":\"ok\"}");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors at high frequency
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    readSensors();
  }
  
  // Process data at medium frequency
  if (currentMillis - lastDataProcess >= DATA_PROCESS_INTERVAL) {
    lastDataProcess = currentMillis;
    processData();
    checkAlerts();
  }
  
  // Sync to cloud at low frequency (or immediately for alerts)
  if (currentMillis - lastCloudSync >= CLOUD_SYNC_INTERVAL || alertHandler.hasPendingAlerts()) {
    lastCloudSync = currentMillis;
    syncToCloud();
  }
  
  // Periodic health check
  if (currentMillis - lastHealthCheck >= HEALTH_CHECK_INTERVAL) {
    lastHealthCheck = currentMillis;
    performHealthCheck();
  }
  
  // Handle any operator gestures
  handleOperatorInput();
}

void readSensors() {
  // Read all sensors and update raw data
  sensorManager.readAll();
  
  // Update current state with latest readings
  currentState.speed_rpm = sensorManager.getConveyorSpeed();
  currentState.conveyorRunning = (currentState.speed_rpm > MIN_SPEED_THRESHOLD);
  currentState.partsPerMinute = sensorManager.getPartsCount();
  currentState.vibrationLevel = sensorManager.getVibrationMagnitude();
  currentState.temperature = sensorManager.getTemperature();
  currentState.humidity = sensorManager.getHumidity();
  currentState.operatorPresent = sensorManager.isOperatorPresent();
}

void processData() {
  // Feed raw data to processor
  dataProcessor.update(currentState);
  
  // Check for anomalies
  if (dataProcessor.detectSpeedAnomaly()) {
    alertHandler.triggerAlert(ALERT_SPEED_ANOMALY, "Speed deviation detected");
  }
  
  if (dataProcessor.detectJam()) {
    currentState.lastJamTime = millis();
    alertHandler.triggerAlert(ALERT_JAM_DETECTED, "Conveyor jam detected");
  }
  
  if (dataProcessor.detectVibrationAnomaly()) {
    alertHandler.triggerAlert(ALERT_VIBRATION_HIGH, "Abnormal vibration detected");
  }
  
  if (dataProcessor.detectEnvironmentalAnomaly()) {
    alertHandler.triggerAlert(ALERT_ENV_CONDITION, "Environmental conditions out of range");
  }
}

void checkAlerts() {
  // Process any pending alerts
  alertHandler.processAlerts(currentState);
}

void syncToCloud() {
  // Prepare telemetry data
  char telemetryData[512];
  snprintf(telemetryData, sizeof(telemetryData),
    "{\"speed_rpm\":%.1f,\"parts_per_min\":%d,\"vibration\":%.2f,"
    "\"temp\":%.1f,\"humidity\":%.1f,\"running\":%s,\"operator\":%s}",
    currentState.speed_rpm,
    currentState.partsPerMinute,
    currentState.vibrationLevel,
    currentState.temperature,
    currentState.humidity,
    currentState.conveyorRunning ? "true" : "false",
    currentState.operatorPresent ? "true" : "false"
  );
  
  // Send regular telemetry
  notecardManager.sendTelemetry(telemetryData);
  
  // Send any pending alerts
  alertHandler.sendPendingAlerts();
}

void handleOperatorInput() {
  GestureType gesture = sensorManager.getLastGesture();
  
  if (gesture != GESTURE_NONE) {
    switch (gesture) {
      case GESTURE_SWIPE_UP:
        // Clear jam acknowledgment
        if (millis() - currentState.lastJamTime < JAM_ACK_WINDOW) {
          alertHandler.acknowledgeAlert(ALERT_JAM_DETECTED);
          notecardManager.sendEvent("operator.action", "{\"action\":\"jam_cleared\"}");
        }
        break;
        
      case GESTURE_SWIPE_DOWN:
        // Pause monitoring
        notecardManager.sendEvent("operator.action", "{\"action\":\"monitoring_paused\"}");
        break;
        
      case GESTURE_WAVE:
        // Resume monitoring
        notecardManager.sendEvent("operator.action", "{\"action\":\"monitoring_resumed\"}");
        break;
    }
    
    sensorManager.clearGesture();
  }
}

void performHealthCheck() {
  // Check sensor connectivity
  if (!sensorManager.checkSensorHealth()) {
    alertHandler.triggerAlert(ALERT_SENSOR_FAILURE, "Sensor communication error");
  }
  
  // Check Notecard connectivity
  if (!notecardManager.isConnected()) {
    // Try to reconnect
    notecardManager.reconnect();
  }
  
  // Log system stats
  Serial.print(F("Health: Speed="));
  Serial.print(currentState.speed_rpm);
  Serial.print(F(" RPM, Parts="));
  Serial.print(currentState.partsPerMinute);
  Serial.print(F("/min, Vib="));
  Serial.print(currentState.vibrationLevel);
  Serial.println(F("g"));
}