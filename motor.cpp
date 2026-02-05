#include "motor.h"
#include "alarms.h"

void sendPelco(byte cmd[]) {
  Serial2.write(cmd, 7);
  // Logowanie ramek Pelco na Serial (debug)
  // for(int i=0; i<7; i++) Serial.printf("%02X ", cmd[i]);
  // Serial.println();
}

// Funkcja sprawdzająca komunikację z rotorem (Ping) - całkowicie wyciszona
void checkRotorComm() {
  // Funkcja wyłączona - rotory CCTV nie posiadają nadajnika RS485 (tryb Listen Only)
  setAlarm(ALARM_COMM_ERROR, false); 
  return; 
}

void stopMotor() {
  sendPelco(pStop);
  motorRunning = false;
  motorDirection = 0;
}

void startMotor(int direction, bool isAZ) {
  // Nie wysyłaj w kółko tej samej komendy (oszczędza RS485 i stabilizuje zliczanie czasu)
  if (motorRunning && motorIsAZ == isAZ && motorDirection == direction) {
    return;
  }
  motorIsAZ = isAZ;
  motorDirection = direction;
  motorStartTime = millis();
  motorRunning = true;
  
  if (isAZ) {
    if (direction > 0) sendPelco(pRight);
    else sendPelco(pLeft);
  } else {
    if (direction > 0) sendPelco(pUp);
    else sendPelco(pDown);
  }
}

void updatePosition() {
  if (!motorRunning || !isCalibrated) return;
  
  unsigned long elapsed = millis() - motorStartTime;
  
  if (motorIsAZ && TR1D_AZ > 0) {
    int degreesMoved = (int)(elapsed / TR1D_AZ);
    if (degreesMoved > 0) {
      if (motorDirection > 0) {
        currentAZ = min(PAN_MAX, currentAZ + degreesMoved);
      } else {
        currentAZ = max(0, currentAZ - degreesMoved);
      }
      motorStartTime = millis();  // Reset timer
      prefs.putInt("currentAZ", currentAZ);  // Zapisz pozycję
    }
  } else if (!motorIsAZ && TR1D_EL > 0) {
    int degreesMoved = (int)(elapsed / TR1D_EL);
    if (degreesMoved > 0) {
      if (motorDirection > 0) {
        currentEL = min(TILT_MAX, currentEL + degreesMoved);
      } else {
        currentEL = max(0, currentEL - degreesMoved);
      }
      motorStartTime = millis();  // Reset timer
      prefs.putInt("currentEL", currentEL);  // Zapisz pozycję
    }
  }
}
