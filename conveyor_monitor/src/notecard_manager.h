#ifndef NOTECARD_MANAGER_H
#define NOTECARD_MANAGER_H

#include <Arduino.h>
#include <Notecard.h>
#include "config.h"

// Notecard Serial configuration
#define NOTECARD_SERIAL Serial1

class NotecardManager {
private:
  Notecard notecard;
  bool connected;
  unsigned long lastSyncTime;
  unsigned long messageCount;
  
  // Connection parameters
  String productUID;
  bool continuousMode;
  int syncMinutes;
  
  // Helper methods
  bool configureNotecard();
  bool setLocationMode();
  
public:
  NotecardManager();
  
  bool begin();
  bool isConnected() { return connected; }
  void reconnect();
  
  // Send data methods
  bool sendTelemetry(const char* jsonData);
  bool sendEvent(const char* eventType, const char* jsonData);
  bool sendAlert(const char* alertType, const char* message, AlertLevel level);
  
  // Configuration
  void setSyncInterval(int minutes);
  void enableMotionDetection(bool enable);
  
  // Status
  bool getSignalStrength(int& rssi, int& bars);
  bool getSyncStatus(unsigned long& lastSync, unsigned long& nextSync);
  unsigned long getMessageCount() { return messageCount; }
};

#endif // NOTECARD_MANAGER_H