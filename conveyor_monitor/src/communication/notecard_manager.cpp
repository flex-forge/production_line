#include "notecard_manager.h"

NotecardManager::NotecardManager() {
  connected = false;
  lastSyncTime = 0;
  messageCount = 0;
  productUID = NOTECARD_PRODUCT_UID;
  continuousMode = NOTECARD_CONTINUOUS;
  syncMinutes = NOTECARD_SYNC_MINS;
}

bool NotecardManager::begin() {
  Serial.println(F("Initializing Notecard..."));

  // Initialize Notecard Serial communication
  notecard.setDebugOutputStream(Serial);
  notecard.begin(NOTECARD_SERIAL, 9600);

  // Configure the Notecard
  if (!configureNotecard()) {
    Serial.println(F("Failed to configure Notecard"));
    return false;
  }

  // Set location mode if needed
  if (!setLocationMode()) {
    Serial.println(F("Failed to set location mode"));
    // Non-critical, continue
  }

  // Enable motion detection if configured
  if (NOTECARD_MOTION_SENSE) {
    enableMotionDetection(true);
  }

  connected = true;
  Serial.println(F("Notecard initialized successfully"));
  return true;
}

bool NotecardManager::configureNotecard() {
  // Set product UID
  J *req = notecard.newRequest("hub.set");
  if (req) {
    JAddStringToObject(req, "product", productUID.c_str());

    if (continuousMode) {
      JAddStringToObject(req, "mode", "continuous");
    } else {
      JAddStringToObject(req, "mode", "periodic");
      JAddNumberToObject(req, "outbound", syncMinutes);
      JAddNumberToObject(req, "inbound", syncMinutes * 2); // Check less frequently
    }

    if (!notecard.sendRequest(req)) {
      return false;
    }
  }

  // Configure card voltage monitoring
  req = notecard.newRequest("card.voltage");
  if (req) {
    JAddStringToObject(req, "mode", "lipo");
    notecard.sendRequest(req);
  }

  // Set up environment variables for the conveyor system
  req = notecard.newRequest("env.set");
  if (req) {
    JAddStringToObject(req, "name", "conveyor_id");
    JAddStringToObject(req, "text", "LINE_001"); // Default line ID
    notecard.sendRequest(req);
  }

  return true;
}

bool NotecardManager::setLocationMode() {
  J *req = notecard.newRequest("card.location.mode");
  if (req) {
    JAddStringToObject(req, "mode", "periodic");
    JAddNumberToObject(req, "seconds", 3600); // Update location hourly
    return notecard.sendRequest(req);
  }
  return false;
}

bool NotecardManager::sendTelemetry(const char* jsonData) {
  if (!connected) {
    return false;
  }

  J *req = notecard.newRequest("note.add");
  if (req) {
    JAddStringToObject(req, "file", "telemetry.qo");
    JAddBoolToObject(req, "sync", false); // Queue for periodic sync
    
    // Parse the JSON data and add fields directly to note body
    J *body = JCreateObject();
    if (body) {
      // Parse the incoming JSON string
      J *telemetryJson = JParse(jsonData);
      if (telemetryJson) {
        // Add each telemetry field directly to the body
        if (JHasObjectItem(telemetryJson, "speed_rpm")) {
          JAddNumberToObject(body, "speed_rpm", JGetNumber(telemetryJson, "speed_rpm"));
        }
        if (JHasObjectItem(telemetryJson, "parts_per_min")) {
          JAddNumberToObject(body, "parts_per_min", JGetNumber(telemetryJson, "parts_per_min"));
        }
        if (JHasObjectItem(telemetryJson, "vibration")) {
          JAddNumberToObject(body, "vibration", JGetNumber(telemetryJson, "vibration"));
        }
        if (JHasObjectItem(telemetryJson, "temp")) {
          JAddNumberToObject(body, "temp", JGetNumber(telemetryJson, "temp"));
        }
        if (JHasObjectItem(telemetryJson, "humidity")) {
          JAddNumberToObject(body, "humidity", JGetNumber(telemetryJson, "humidity"));
        }
        if (JHasObjectItem(telemetryJson, "pressure")) {
          JAddNumberToObject(body, "pressure", JGetNumber(telemetryJson, "pressure"));
        }
        if (JHasObjectItem(telemetryJson, "gas_resistance")) {
          JAddNumberToObject(body, "gas_resistance", JGetNumber(telemetryJson, "gas_resistance"));
        }
        if (JHasObjectItem(telemetryJson, "running")) {
          JAddBoolToObject(body, "running", JGetBool(telemetryJson, "running"));
        }
        if (JHasObjectItem(telemetryJson, "operator")) {
          JAddBoolToObject(body, "operator", JGetBool(telemetryJson, "operator"));
        }
        
        JDelete(telemetryJson);
      }
      
      // Add timestamp
      JAddNumberToObject(body, "time", millis() / 1000);
      
      JAddItemToObject(req, "body", body);
      
      if (notecard.sendRequest(req)) {
        messageCount++;
        return true;
      }
    }
  }
  return false;
}

bool NotecardManager::sendEvent(const char* eventType, const char* jsonData) {
  if (!connected) {
    return false;
  }
  
  J *req = notecard.newRequest("note.add");
  if (req) {
    JAddStringToObject(req, "file", "events.qo");
    JAddBoolToObject(req, "sync", true); // Sync immediately for events
    
    J *body = JCreateObject();
    if (body) {
      JAddStringToObject(body, "event", eventType);
      JAddNumberToObject(body, "time", millis() / 1000);
      
      // Parse and add data as an object instead of string
      if (jsonData && strlen(jsonData) > 0) {
        J *dataJson = JParse(jsonData);
        if (dataJson) {
          JAddItemToObject(body, "data", dataJson);
        }
      }
      
      JAddItemToObject(req, "body", body);
      
      if (notecard.sendRequest(req)) {
        messageCount++;
        lastSyncTime = millis();
        return true;
      }
    }
  }
  return false;
}

bool NotecardManager::sendAlert(const char* alertType, const char* message, AlertLevel level) {
  if (!connected) {
    return false;
  }
  
  J *req = notecard.newRequest("note.add");
  if (req) {
    JAddStringToObject(req, "file", "alerts.qo");
    JAddBoolToObject(req, "sync", true); // Always sync alerts immediately
    JAddBoolToObject(req, "urgent", level >= ALERT_CRITICAL);
    
    J *body = JCreateObject();
    if (body) {
      JAddStringToObject(body, "alert", alertType);
      JAddStringToObject(body, "message", message);
      JAddNumberToObject(body, "level", level);
      JAddNumberToObject(body, "time", millis() / 1000);
      
      JAddItemToObject(req, "body", body);
      
      if (notecard.sendRequest(req)) {
        messageCount++;
        lastSyncTime = millis();
        return true;
      }
    }
  }
  return false;
}

void NotecardManager::reconnect() {
  Serial.println(F("Attempting Notecard reconnection..."));
  
  // Try to sync
  J *req = notecard.newRequest("hub.sync");
  if (req) {
    if (notecard.sendRequest(req)) {
      connected = true;
      lastSyncTime = millis();
      Serial.println(F("Notecard reconnected"));
    } else {
      connected = false;
      Serial.println(F("Notecard reconnection failed"));
    }
  }
}

void NotecardManager::setSyncInterval(int minutes) {
  syncMinutes = minutes;
  
  J *req = notecard.newRequest("hub.set");
  if (req) {
    JAddNumberToObject(req, "outbound", minutes);
    JAddNumberToObject(req, "inbound", minutes * 2);
    notecard.sendRequest(req);
  }
}

void NotecardManager::enableMotionDetection(bool enable) {
  J *req = notecard.newRequest("card.motion.mode");
  if (req) {
    JAddBoolToObject(req, "start", enable);
    if (enable) {
      JAddNumberToObject(req, "sensitivity", 2); // Medium sensitivity
      JAddNumberToObject(req, "seconds", 30); // Trigger after 30s of motion
    }
    notecard.sendRequest(req);
  }
}

bool NotecardManager::getSignalStrength(int& rssi, int& bars) {
  J *req = notecard.newRequest("card.wireless");
  J *rsp = notecard.requestAndResponse(req);
  
  if (rsp) {
    rssi = JGetNumber(rsp, "rssi");
    bars = JGetNumber(rsp, "bars");
    notecard.deleteResponse(rsp);
    return true;
  }
  return false;
}

bool NotecardManager::getSyncStatus(unsigned long& lastSync, unsigned long& nextSync) {
  J *req = notecard.newRequest("hub.sync.status");
  J *rsp = notecard.requestAndResponse(req);
  
  if (rsp) {
    lastSync = JGetNumber(rsp, "time") * 1000; // Convert to millis
    nextSync = JGetNumber(rsp, "next") * 1000;
    notecard.deleteResponse(rsp);
    return true;
  }
  return false;
}