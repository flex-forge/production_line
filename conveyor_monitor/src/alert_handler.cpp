#include "alert_handler.h"

AlertHandler::AlertHandler() {
  alertCount = 0;
  notecard = nullptr;
  
  // Initialize tracking arrays
  for (int i = 0; i < 10; i++) {
    lastAlertTime[i] = 0;
    alertFrequency[i] = 0;
  }
}

void AlertHandler::begin(NotecardManager* nc) {
  notecard = nc;
  Serial.println(F("Alert handler initialized"));
}

void AlertHandler::triggerAlert(AlertType type, const char* message) {
  // Check if we should suppress this alert
  if (shouldSuppressAlert(type)) {
    return;
  }
  
  // Determine alert level based on type and frequency
  AlertLevel level = determineAlertLevel(type);
  
  // Add to alert queue
  addAlert(type, level, String(message));
  
  // Update tracking
  lastAlertTime[type] = millis();
  alertFrequency[type]++;
  
  // Log locally
  Serial.print(F("ALERT ["));
  Serial.print(level == ALERT_CRITICAL ? "CRITICAL" : 
               level == ALERT_WARNING ? "WARNING" : "INFO");
  Serial.print(F("]: "));
  Serial.println(message);
}

AlertLevel AlertHandler::determineAlertLevel(AlertType type) {
  // Base level by type
  AlertLevel baseLevel = ALERT_INFO;
  
  switch (type) {
    case ALERT_JAM_DETECTED:
    case ALERT_SENSOR_FAILURE:
    case ALERT_COMM_FAILURE:
      baseLevel = ALERT_CRITICAL;
      break;
      
    case ALERT_SPEED_ANOMALY:
    case ALERT_VIBRATION_HIGH:
      baseLevel = ALERT_WARNING;
      break;
      
    case ALERT_ENV_CONDITION:
      baseLevel = ALERT_INFO;
      break;
  }
  
  // Escalate if frequent
  if (alertFrequency[type] > 5 && baseLevel < ALERT_CRITICAL) {
    return ALERT_CRITICAL;
  } else if (alertFrequency[type] > 3 && baseLevel < ALERT_WARNING) {
    return ALERT_WARNING;
  }
  
  return baseLevel;
}

bool AlertHandler::shouldSuppressAlert(AlertType type) {
  unsigned long currentTime = millis();
  unsigned long timeSinceLastAlert = currentTime - lastAlertTime[type];
  
  // Suppress if too frequent (except critical alerts)
  if (determineAlertLevel(type) != ALERT_CRITICAL) {
    if (timeSinceLastAlert < 60000) { // Within 1 minute
      return true;
    }
  } else {
    // Even critical alerts shouldn't spam
    if (timeSinceLastAlert < 5000) { // Within 5 seconds
      return true;
    }
  }
  
  return false;
}

void AlertHandler::addAlert(AlertType type, AlertLevel level, const String& message) {
  // Find existing alert of same type or add new
  int index = -1;
  for (int i = 0; i < alertCount; i++) {
    if (alerts[i].type == type && !alerts[i].acknowledged) {
      index = i;
      break;
    }
  }
  
  if (index >= 0) {
    // Update existing alert
    alerts[index].message = message;
    alerts[index].timestamp = millis();
    alerts[index].sent = false;
  } else if (alertCount < MAX_ALERTS) {
    // Add new alert
    alerts[alertCount].type = type;
    alerts[alertCount].level = level;
    alerts[alertCount].message = message;
    alerts[alertCount].timestamp = millis();
    alerts[alertCount].acknowledged = false;
    alerts[alertCount].sent = false;
    alertCount++;
  }
}

void AlertHandler::acknowledgeAlert(AlertType type) {
  for (int i = 0; i < alertCount; i++) {
    if (alerts[i].type == type) {
      alerts[i].acknowledged = true;
      
      // Send acknowledgment to cloud
      if (notecard) {
        char data[128];
        snprintf(data, sizeof(data), "{\"alert_type\":\"%d\",\"action\":\"acknowledged\"}", type);
        notecard->sendEvent("alert.acknowledged", data);
      }
      
      Serial.print(F("Alert acknowledged: "));
      Serial.println(alerts[i].message);
      break;
    }
  }
}

void AlertHandler::clearAlert(AlertType type) {
  // Remove alert from active list
  for (int i = 0; i < alertCount; i++) {
    if (alerts[i].type == type) {
      // Shift remaining alerts
      for (int j = i; j < alertCount - 1; j++) {
        alerts[j] = alerts[j + 1];
      }
      alertCount--;
      break;
    }
  }
  
  // Reset frequency counter
  alertFrequency[type] = 0;
}

void AlertHandler::processAlerts(const SystemState& state) {
  // Auto-clear certain alerts based on state
  
  // Clear jam alert if conveyor is running normally
  if (state.conveyorRunning && state.partsPerMinute > 0) {
    for (int i = 0; i < alertCount; i++) {
      if (alerts[i].type == ALERT_JAM_DETECTED && !alerts[i].acknowledged) {
        clearAlert(ALERT_JAM_DETECTED);
        break;
      }
    }
  }
  
  // Clear speed anomaly if speed is normal
  if (abs(state.speed_rpm - NOMINAL_SPEED_RPM) < (NOMINAL_SPEED_RPM * SPEED_TOLERANCE_PCT / 100)) {
    clearAlert(ALERT_SPEED_ANOMALY);
  }
  
  // Clear environmental alert if conditions are normal
  if (state.temperature >= TEMP_MIN_C && state.temperature <= TEMP_MAX_C &&
      state.humidity <= HUMIDITY_MAX_PCT) {
    clearAlert(ALERT_ENV_CONDITION);
  }
}

void AlertHandler::sendPendingAlerts() {
  if (!notecard) return;
  
  for (int i = 0; i < alertCount; i++) {
    if (!alerts[i].sent && !alerts[i].acknowledged) {
      // Prepare alert data
      char alertData[256];
      snprintf(alertData, sizeof(alertData),
        "{\"type\":%d,\"level\":%d,\"message\":\"%s\",\"timestamp\":%lu}",
        alerts[i].type,
        alerts[i].level,
        alerts[i].message.c_str(),
        alerts[i].timestamp
      );
      
      // Send based on level
      const char* alertTypeStr = "";
      switch (alerts[i].type) {
        case ALERT_SPEED_ANOMALY: alertTypeStr = "speed_anomaly"; break;
        case ALERT_JAM_DETECTED: alertTypeStr = "jam_detected"; break;
        case ALERT_VIBRATION_HIGH: alertTypeStr = "vibration_high"; break;
        case ALERT_ENV_CONDITION: alertTypeStr = "environmental"; break;
        case ALERT_SENSOR_FAILURE: alertTypeStr = "sensor_failure"; break;
        case ALERT_COMM_FAILURE: alertTypeStr = "comm_failure"; break;
      }
      
      if (notecard->sendAlert(alertTypeStr, alerts[i].message.c_str(), alerts[i].level)) {
        alerts[i].sent = true;
      }
    }
  }
}

bool AlertHandler::hasPendingAlerts() {
  for (int i = 0; i < alertCount; i++) {
    if (!alerts[i].sent && !alerts[i].acknowledged) {
      return true;
    }
  }
  return false;
}

int AlertHandler::getActiveAlertCount() {
  int count = 0;
  for (int i = 0; i < alertCount; i++) {
    if (!alerts[i].acknowledged) {
      count++;
    }
  }
  return count;
}

Alert* AlertHandler::getActiveAlerts() {
  return alerts;
}