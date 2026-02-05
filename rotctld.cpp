#include "rotctld.h"
#include "position.h"
#include "motor.h"
#include "alarms.h"

// Obsługa komend GS-232A/B (tylko dla Serial USB)
void handleGs232Command(char* cmd, int len, Stream* output, bool logSerial) {
  // Usuń białe znaki na końcu
  while (len > 0 && (cmd[len-1] == '\r' || cmd[len-1] == '\n' || cmd[len-1] == ' ')) {
    cmd[--len] = '\0';
  }
  if (len <= 0) return;

  if (logSerial) {
    Serial.printf("[GS232 RX] %s\n", cmd);
  }

  if ((cmd[0] == 'C' && len == 1) || (cmd[0] == 'C' && cmd[1] == '2' && len == 2)) {
    // Zwróć azymut geograficzny (0-359) i elewację (Standard Yaesu GS-232A/B)
    int geoAZ = getGeographicAzimuth();
    int userEL = getUserElevation();
    char resp[32];
    snprintf(resp, sizeof(resp), "AZ=%03d EL=%02d\r\n", geoAZ, userEL);
    if (output) output->print(resp);
    if (logSerial) Serial.printf("[GS232 TX] %s", resp);
  } else if (cmd[0] == 'M' && len > 1) {
    // Komenda Maaa - ustaw Azymut (000-359)
    int val = atoi(&cmd[1]);
    if (val >= 0 && val <= 359) {
      if (isAutoMode) {
        targetAZ = geographicToInternal(val);
      } else {
        if (millis() - lastAutoNotificationTime > 30000) {
          showNotification("PRZELACZ NA AUTO!");
          lastAutoNotificationTime = millis();
        }
      }
    }
  } else if (cmd[0] == 'B' && len > 1) {
    // Komenda Beee - ustaw Elewację (000-090)
    int val = atoi(&cmd[1]);
    if (val >= 0 && val <= 90) {
      if (isAutoMode) {
        targetEL = userToInternalEL(val);
      } else {
        if (millis() - lastAutoNotificationTime > 30000) {
          showNotification("PRZELACZ NA AUTO!");
          lastAutoNotificationTime = millis();
        }
      }
    }
  } else if (cmd[0] == 'W' && len >= 7) {
    // Komenda Waaa eee - ustaw Azymut i Elewację
    int az, el;
    // Format Waaa eee (np. W120 045)
    if (sscanf(cmd, "W%d %d", &az, &el) == 2) {
      if (isAutoMode) {
        targetAZ = geographicToInternal(az % 360);
        targetEL = userToInternalEL(el);
      } else {
        if (millis() - lastAutoNotificationTime > 30000) {
          showNotification("PRZELACZ NA AUTO!");
          lastAutoNotificationTime = millis();
        }
      }
    }
  } else if (cmd[0] == 'S' && len == 1) {
    // Komenda S - Stop wszystko
    if (isAutoMode) {
      targetAZ = currentAZ;
      targetEL = currentEL;
      stopMotor();
    }
  } else if (cmd[0] == 'A' && len == 1) {
    // Komenda A - Stop Azymut
    if (isAutoMode) {
      targetAZ = currentAZ;
      if (motorRunning && motorIsAZ) stopMotor();
    }
  } else if (cmd[0] == 'E' && len == 1) {
    // Komenda E - Stop Elewacja
    if (isAutoMode) {
      targetEL = currentEL;
      if (motorRunning && !motorIsAZ) stopMotor();
    }
  } else if (cmd[0] == 'L' && len == 1) {
    // Komenda L - Obrót w lewo (manualny start z PC)
    if (isAutoMode) startMotor(-1, true);
    else {
      if (millis() - lastAutoNotificationTime > 30000) {
        showNotification("PRZELACZ NA AUTO!");
        lastAutoNotificationTime = millis();
      }
    }
  } else if (cmd[0] == 'R' && len == 1) {
    // Komenda R - Obrót w prawo
    if (isAutoMode) startMotor(1, true);
    else {
      if (millis() - lastAutoNotificationTime > 30000) {
        showNotification("PRZELACZ NA AUTO!");
        lastAutoNotificationTime = millis();
      }
    }
  } else if (cmd[0] == 'U' && len == 1) {
    // Komenda U - Obrót w górę
    if (isAutoMode) startMotor(1, false);
    else {
      if (millis() - lastAutoNotificationTime > 30000) {
        showNotification("PRZELACZ NA AUTO!");
        lastAutoNotificationTime = millis();
      }
    }
  } else if (cmd[0] == 'D' && len == 1) {
    // Komenda D - Obrót w dół
    if (isAutoMode) startMotor(-1, false);
    else {
      if (millis() - lastAutoNotificationTime > 30000) {
        showNotification("PRZELACZ NA AUTO!");
        lastAutoNotificationTime = millis();
      }
    }
  }
}

// Obsługa komend rotctld/Hamlib (tylko dla TCP/IP)
void handleRotctldCommand(char* cmd, int len, Stream* output, bool logSerial) {
  // Usuń białe znaki na końcu
  while (len > 0 && (cmd[len-1] == '\r' || cmd[len-1] == '\n' || cmd[len-1] == ' ' || cmd[len-1] == '\t')) {
    cmd[--len] = '\0';
  }
  if (len <= 0) return;

  // Zamień przecinki na kropki (obsługa różnych lokalizacji oprogramowania PC)
  for (int i = 0; i < len; i++) {
    if (cmd[i] == ',') cmd[i] = '.';
  }

  if (logSerial) {
    Serial.printf("[ROTCTLD RX] %s\n", cmd);
  }

  bool extendedResponse = false;
  char* actualCmd = cmd;

  // Sprawdź czy jest prefiks Protocol (+, ;, |, \)
  if (len > 0 && (cmd[0] == '+' || cmd[0] == ';' || cmd[0] == '|' || cmd[0] == '\\')) {
    // Jeśli prefiks to '+', ';', '|' -> włącz tryb Extended
    if (cmd[0] != '\\') extendedResponse = true;
    
    actualCmd = &cmd[1];
    len--;
  }

  // Komenda get_pos: p lub \get_pos
  if ((actualCmd[0] == 'p' && len == 1) || (len >= 8 && strncmp(actualCmd, "\\get_pos", 8) == 0)) {
    int geoAZ = getGeographicAzimuth();
    int userEL = getUserElevation();
    float azFloat = (float)geoAZ;
    float elFloat = (float)userEL;
    
    if (extendedResponse) {
      // Extended Response Protocol
      char resp[128];
      snprintf(resp, sizeof(resp), "get_pos:\nAzimuth: %.6f\nElevation: %.6f\nRPRT 0\n", azFloat, elFloat);
      if (output) output->print(resp);
      if (logSerial) Serial.printf("[ROTCTLD TX] %s", resp);
    } else {
      // Standard Response (dwie linie z wartościami)
      char resp[64];
      snprintf(resp, sizeof(resp), "%.6f\n%.6f\n", azFloat, elFloat);
      if (output) output->print(resp);
      if (logSerial) Serial.printf("[ROTCTLD TX] %s", resp);
    }
  }
  // Komenda set_pos: P az el lub \set_pos az el
  else if (actualCmd[0] == 'P' && len > 1) {
    // Format: P az el (np. P0.4 45.4 lub P 90 45)
    float az, el;
    if (sscanf(&actualCmd[1], "%f %f", &az, &el) == 2) {
      if (isAutoMode) {
        int azInt = (int)(az + 0.5f) % 360;  // Zaokrąglij i ogranicz do 0-359
        int elInt = (int)(el + 0.5f);
        elInt = constrain(elInt, 0, 90);

        targetAZ = geographicToInternal(azInt);
        targetEL = userToInternalEL(elInt);

        if (extendedResponse) {
          if (output) output->print("RPRT 0\n");
        }
      } else {
        if (extendedResponse) {
          if (output) output->print("RPRT -1 (Not in AUTO mode)\n");
        }
        // Pokazuj powiadomienie nie częściej niż raz na 30 sekund
        if (millis() - lastAutoNotificationTime > 30000) {
          showNotification("PRZELACZ NA AUTO!");
          lastAutoNotificationTime = millis();
        }
      }
    } else {
      if (logSerial) Serial.println("[ROTCTLD] Blad parsowania komendy P (sprawdz format)");
    }
  }
  else if (len >= 8 && strncmp(actualCmd, "\\set_pos", 8) == 0) {
    // Format: \set_pos az el
    float az, el;
    if (sscanf(&actualCmd[8], " %f %f", &az, &el) == 2) {
      if (isAutoMode) {
        int azInt = (int)(az + 0.5f) % 360;
        int elInt = (int)(el + 0.5f);
        elInt = constrain(elInt, 0, 90);

        targetAZ = geographicToInternal(azInt);
        targetEL = userToInternalEL(elInt);

        if (extendedResponse) {
          if (output) output->print("RPRT 0\n");
        }
      } else {
        if (extendedResponse) {
          if (output) output->print("RPRT -1 (Not in AUTO mode)\n");
        }
        // Pokazuj powiadomienie nie częściej niż raz na 30 sekund
        if (millis() - lastAutoNotificationTime > 30000) {
          showNotification("PRZELACZ NA AUTO!");
          lastAutoNotificationTime = millis();
        }
      }
    } else {
      if (logSerial) Serial.println("[ROTCTLD] Blad parsowania komendy \\set_pos");
    }
  }
  // Komenda stop: S lub \stop
  else if ((actualCmd[0] == 'S' && len == 1) || (len >= 5 && strncmp(actualCmd, "\\stop", 5) == 0)) {
    if (isAutoMode) {
      targetAZ = currentAZ;
      targetEL = currentEL;
      stopMotor();
      if (extendedResponse) {
        if (output) output->print("RPRT 0\n");
      }
    }
  }
  // Inne komendy rotctld można dodać tutaj w przyszłości
  else {
    // Nieznana komenda - zwróć błąd w trybie extended
    if (extendedResponse) {
      if (output) output->print("RPRT -1\n");  // -1 = błąd
    }
    if (logSerial) {
      Serial.printf("[ROTCTLD] Unknown command: %s\n", cmd);
    }
  }
}
