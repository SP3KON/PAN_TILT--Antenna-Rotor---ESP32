#ifndef ALARMS_H
#define ALARMS_H

#include "config.h"

// System alarmów
void showNotification(const char* msg);
void setAlarm(AlarmType type, bool active);
void checkAlarms();

#endif // ALARMS_H
