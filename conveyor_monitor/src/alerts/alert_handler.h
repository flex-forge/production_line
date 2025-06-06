#ifndef ALERT_HANDLER_H
#define ALERT_HANDLER_H

#include <Arduino.h>
#include "../config/config.h"
#include "../communication/notecard_manager.h"

struct Alert {
  AlertType type;
  AlertLevel level;
  String message;
  unsigned long timestamp;
  bool acknowledged;
  bool sent;
};

class AlertHandler {
private:
  static const int MAX_ALERTS = 10;
  Alert alerts[MAX_ALERTS];
  int alertCount;
  NotecardManager* notecard;
  
  // Alert state tracking
  unsigned long lastAlertTime[10]; // Track last occurrence of each alert type
  int alertFrequency[10]; // Count occurrences
  
  // Helper methods
  AlertLevel determineAlertLevel(AlertType type);
  bool shouldSuppressAlert(AlertType type);
  void addAlert(AlertType type, AlertLevel level, const String& message);
  
public:
  AlertHandler();
  
  void begin(NotecardManager* nc);
  
  // Alert management
  void triggerAlert(AlertType type, const char* message);
  void acknowledgeAlert(AlertType type);
  void clearAlert(AlertType type);
  
  // Process and send alerts
  void processAlerts(const SystemState& state);
  void sendPendingAlerts();
  bool hasPendingAlerts();
  
  // Get alert info
  int getActiveAlertCount();
  Alert* getActiveAlerts();
};

#endif // ALERT_HANDLER_H