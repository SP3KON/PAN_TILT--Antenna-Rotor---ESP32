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

void autoPositionControl() {
  if (isCalibrated) {
    // AUTO: target z aplikacji (BT). MAN: target z enkodera.
    // Sterowanie do targetu działa w obu trybach.
    int diffAZ = targetAZ - currentAZ;
    int diffEL = targetEL - currentEL;

    // Mechanizm zabezpieczający przed oscylacjami dla AZ
    if (targetAZ != oscProtectAZ.lastTargetAZ) {
      // Nowy cel - resetuj licznik prób
      oscProtectAZ.attemptCount = 0;
      oscProtectAZ.bestPosition = currentAZ;
      oscProtectAZ.bestDiff = abs(diffAZ);
      oscProtectAZ.lastDirection = 0;
      oscProtectAZ.lastTargetAZ = targetAZ;
    } else if (motorIsAZ && motorRunning) {
      // Sprawdź czy przekroczyliśmy cel podczas ruchu
      bool overshot = false;
      if (motorDirection > 0 && currentAZ > targetAZ) {
        // Jedziemy w prawo i przekroczyliśmy cel
        overshot = true;
      } else if (motorDirection < 0 && currentAZ < targetAZ) {
        // Jedziemy w lewo i przekroczyliśmy cel
        overshot = true;
      }
      
      if (overshot && oscProtectAZ.lastDirection == motorDirection) {
        // Przekroczyliśmy cel w tym samym kierunku co ostatnio = próba
        oscProtectAZ.attemptCount++;
        Serial.printf("[OSC PROTECT AZ] Przekroczono cel - proba %d/3 (poz: %d, cel: %d)\n", 
                   oscProtectAZ.attemptCount, currentAZ, targetAZ);
        
        if (oscProtectAZ.attemptCount >= 3) {
          // Po 3 próbach - zatrzymaj się na najlepszej pozycji
          Serial.printf("[OSC PROTECT AZ] Osiagnieto limit 3 prob - zatrzymanie na pozycji %d (cel: %d)\n", 
                       oscProtectAZ.bestPosition, targetAZ);
          stopMotor();
          // Ustaw target na najlepszą pozycję (zatrzymaj się)
          targetAZ = oscProtectAZ.bestPosition;
          currentAZ = oscProtectAZ.bestPosition;
          prefs.putInt("currentAZ", currentAZ);
          oscProtectAZ.attemptCount = 0;  // Reset dla następnego celu
          return;  // Wyjdź z funkcji - nie próbuj już jechać
        }
      }
      
      if (overshot) {
        oscProtectAZ.lastDirection = motorDirection;
      }
      
      // Aktualizuj najlepszą pozycję (najbliższą do celu)
      if (abs(diffAZ) < oscProtectAZ.bestDiff) {
        oscProtectAZ.bestPosition = currentAZ;
        oscProtectAZ.bestDiff = abs(diffAZ);
      }
    } else if (abs(diffAZ) <= 1) {
      // Osiągnięto cel - resetuj licznik prób
      oscProtectAZ.attemptCount = 0;
      oscProtectAZ.lastDirection = 0;
    }

    // Mechanizm zabezpieczający przed oscylacjami dla EL
    if (targetEL != oscProtectEL.lastTargetEL) {
      // Nowy cel - resetuj licznik prób
      oscProtectEL.attemptCount = 0;
      oscProtectEL.bestPosition = currentEL;
      oscProtectEL.bestDiff = abs(diffEL);
      oscProtectEL.lastDirection = 0;
      oscProtectEL.lastTargetEL = targetEL;
    } else if (!motorIsAZ && motorRunning) {
      // Sprawdź czy przekroczyliśmy cel podczas ruchu
      bool overshot = false;
      if (motorDirection > 0 && currentEL > targetEL) {
        // Jedziemy w górę i przekroczyliśmy cel
        overshot = true;
      } else if (motorDirection < 0 && currentEL < targetEL) {
        // Jedziemy w dół i przekroczyliśmy cel
        overshot = true;
      }
      
      if (overshot && oscProtectEL.lastDirection == motorDirection) {
        // Przekroczyliśmy cel w tym samym kierunku co ostatnio = próba
        oscProtectEL.attemptCount++;
        Serial.printf("[OSC PROTECT EL] Przekroczono cel - proba %d/3 (poz: %d, cel: %d)\n", 
                     oscProtectEL.attemptCount, currentEL, targetEL);
        
        if (oscProtectEL.attemptCount >= 3) {
          // Po 3 próbach - zatrzymaj się na najlepszej pozycji
          Serial.printf("[OSC PROTECT EL] Osiagnieto limit 3 prob - zatrzymanie na pozycji %d (cel: %d)\n", 
                       oscProtectEL.bestPosition, targetEL);
          stopMotor();
          // Ustaw target na najlepszą pozycję
          targetEL = oscProtectEL.bestPosition;
          currentEL = oscProtectEL.bestPosition;
          prefs.putInt("currentEL", currentEL);
          oscProtectEL.attemptCount = 0;
          return;  // Wyjdź z funkcji
        }
      }
      
      if (overshot) {
        oscProtectEL.lastDirection = motorDirection;
      }
      
      // Aktualizuj najlepszą pozycję
      if (abs(diffEL) < oscProtectEL.bestDiff) {
        oscProtectEL.bestPosition = currentEL;
        oscProtectEL.bestDiff = abs(diffEL);
      }
    } else if (abs(diffEL) <= 1) {
      // Osiągnięto cel - resetuj licznik prób
      oscProtectEL.attemptCount = 0;
      oscProtectEL.lastDirection = 0;
    }

    if (abs(diffAZ) > 1) {
      // Najpierw AZ
      if (!motorRunning || motorIsAZ) {
        startMotor((diffAZ > 0) ? 1 : -1, true);
      }
    } else if (abs(diffEL) > 1) {
      // Potem EL
      if (!motorRunning || !motorIsAZ) {
        startMotor((diffEL > 0) ? 1 : -1, false);
      }
  } else {
      // Osiągnięto cel - zatrzymaj tylko jeśli silnik był uruchomiony
      if (motorRunning) {
        stopMotor();
      }
    }
  } else {
    // Brak kalibracji - zatrzymaj silnik (tylko raz jeśli był uruchomiony)
    if (motorRunning) {
      stopMotor();
    }
  }
  
  // LOGIKA LED: Miga podczas obrotu, świeci gdy stoi
  if (motorRunning) {
    digitalWrite(PIN_LED, (millis() / 200) % 2);  // Migaj podczas obrotu
  } else {
    digitalWrite(PIN_LED, HIGH);  // Świeci ciągle gdy stoi
  }
}
