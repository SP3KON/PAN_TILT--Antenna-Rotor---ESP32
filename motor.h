#ifndef MOTOR_H
#define MOTOR_H

#include "config.h"

// Motor control functions
void sendPelco(byte cmd[]);
void checkRotorComm();
void stopMotor();
void startMotor(int direction, bool isAZ);
void updatePosition();
void autoPositionControl();

#endif // MOTOR_H
