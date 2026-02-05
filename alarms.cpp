#include "alarms.h"

void showNotification(const char* msg) {
  notificationActive = true;
  notificationMessage = msg;
  notificationStartTime = millis();
}

void setAlarm(AlarmType type, bool active) {
  for (int i = 0; i < MAX_ALARMS - 1; i++) {
    if (alarms[i].type == type) {
      if (alarms[i].active != active) {
        alarms[i].active = active;
        if (active) {
          Serial.printf("[ALARM] %s - AKTYWNY\n", alarms[i].message);
          showNotification(alarms[i].message);
        } else {
          Serial.printf("[ALARM] %s - WYLACZONY\n", alarms[i].message);
        }
      }
      return;
    }
  }
}

void checkAlarms() {
  // Sprawdź brak kalibracji
  setAlarm(ALARM_NO_CALIBRATION, !isCalibrated);
  
  // Tutaj można dodać inne sprawdzenia alarmów
  // setAlarm(ALARM_MOTOR_ERROR, ...);
  // setAlarm(ALARM_COMM_ERROR, ...);
}
