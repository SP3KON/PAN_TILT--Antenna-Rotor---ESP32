#include "display_menu.h"
#include "config.h"
#include "position.h"
#include "motor.h"
#include "calibration.h"

void handleEncoderAndButton() {
  // Obsługa przycisku enkodera
  static bool lastSw = HIGH;
  bool sw = digitalRead(PIN_ENC_SW);
  
  if (lastSw == HIGH && sw == LOW) {
    // Debounce przycisku
    delay(BTN_DEBOUNCE_MS);
    if (digitalRead(PIN_ENC_SW) != LOW) {
      // Fałszywe zbocze (drgania)
      lastSw = digitalRead(PIN_ENC_SW);
    } else {
    unsigned long start = millis();
    while(digitalRead(PIN_ENC_SW) == LOW) {
      delay(10);
    }
    unsigned long dur = millis() - start;
    
    if (dur > 400) {
      // Długie naciśnięcie - powrót do menu głównego lub wyjście z podmenu
      if (currentMenu == CALIB_AZ_CONFIRM || currentMenu == CALIB_EL_CONFIRM) {
        currentMenu = MENU_ROOT;
      } else       if (currentMenu == ROTOR_SETTINGS || currentMenu == SET_PAN || 
          currentMenu == SET_TILT || currentMenu == SET_COMM_SPEED || 
          currentMenu == SET_DIRECTION || currentMenu == REMOTE_CTRL || 
          currentMenu == REMOTE_CTRL_SELECT) {
        currentMenu = MENU_ROOT;
      } else if (currentMenu != MAIN_SCR) {
        stopMotor();
        currentMenu = MAIN_SCR;
      }
    } else {
      // Krótkie naciśnięcie - akcja
      if (currentMenu == MAIN_SCR) {
        currentMenu = MENU_ROOT;
      } else if (currentMenu == MENU_ROOT) {
        if (menuIdx == 0) { 
          isAutoMode = !isAutoMode; 
          prefs.putBool("isAutoMode", isAutoMode);
        }
        else if (menuIdx == 1) currentMenu = ROTOR_SETTINGS;
        else if (menuIdx == 2) currentMenu = CALIB_AZ_CONFIRM;
        else if (menuIdx == 3) currentMenu = CALIB_EL_CONFIRM;
        else if (menuIdx == 4) currentMenu = ALARMS_MENU;
        else if (menuIdx == 5) currentMenu = INFO_SCR;
        else if (menuIdx == 6) currentMenu = REMOTE_CTRL;
      } else if (currentMenu == CALIB_AZ_CONFIRM) {
        calibrateAZ();
        currentMenu = MAIN_SCR;
        isCalibrated = (TR1D_AZ > 0 && TR1D_EL > 0);
      } else if (currentMenu == CALIB_EL_CONFIRM) {
        calibrateEL();
        currentMenu = MAIN_SCR;
        isCalibrated = (TR1D_AZ > 0 && TR1D_EL > 0);
      } else if (currentMenu == ROTOR_SETTINGS) {
        if (rotorMenuIdx == 0) currentMenu = SET_PAN;
        else if (rotorMenuIdx == 1) currentMenu = SET_TILT;
        else if (rotorMenuIdx == 2) currentMenu = SET_COMM_SPEED;
        else if (rotorMenuIdx == 3) currentMenu = SET_DIRECTION;
        else if (rotorMenuIdx == 4) currentMenu = SET_PAN_DIR;
        else if (rotorMenuIdx == 5) currentMenu = SET_TILT_DIR;
      } else if (currentMenu == SET_PAN || currentMenu == SET_TILT || 
                 currentMenu == SET_COMM_SPEED || currentMenu == SET_DIRECTION ||
                 currentMenu == SET_PAN_DIR || currentMenu == SET_TILT_DIR) {
        // Zapisz ustawienia i wróć do menu ROTOR SETTINGS
        prefs.putInt("PAN_MAX", PAN_MAX);
        prefs.putInt("TILT_MAX", TILT_MAX);
        prefs.putInt("commSpeed", commSpeed);
        prefs.putInt("direction", direction);
        prefs.putBool("reversePAN", reversePAN);
        prefs.putBool("reverseTILT", reverseTILT);
        
        // Jeśli zmieniono prędkość komunikacji, zrestartuj Serial2
        if (currentMenu == SET_COMM_SPEED) {
          Serial2.end();
          delay(100);
          Serial2.begin(commSpeed, SERIAL_8N1, RS485_RX, RS485_TX);
        }
        
        currentMenu = ROTOR_SETTINGS;
      } else if (currentMenu == REMOTE_CTRL) {
        if (remoteMenuIdx == 0) {
          remoteCtrlMode = REMOTE_USBSERIAL;
          prefs.putInt("remoteCtrlMode", remoteCtrlMode);
          display.clearDisplay();
          display.setCursor(0, 20);
          display.print("Tryb: USBSERIAL");
          display.setCursor(0, 40);
          display.print("Restartowanie...");
          display.display();
          delay(2000);
          ESP.restart();
        } else if (remoteMenuIdx == 1) {
          remoteCtrlMode = REMOTE_TCPIP;
          prefs.putInt("remoteCtrlMode", remoteCtrlMode);
          display.clearDisplay();
          display.setCursor(0, 20);
          display.print("Tryb: TCPIP");
          display.setCursor(0, 40);
          display.print("Restartowanie...");
          display.display();
          delay(2000);
          ESP.restart();
        }
      }
    }
    }
  }
  lastSw = digitalRead(PIN_ENC_SW);

  // Enkoder
  static long lPos = 0;
  static long encRemainder = 0;
  static unsigned long lastEncoderMove = 0;
  static unsigned long lastEncoderStepMs = 0;

  long pos = encoder.getCount();
  long rawDelta = pos - lPos;
  if (rawDelta != 0) {
    encRemainder += rawDelta;
    lPos = pos;
  }

  int d = 0; // d = liczba "kroków" enkodera (detentów), dodatnia/ujemna
  unsigned long now = millis();
  if (abs(encRemainder) >= ENCODER_COUNTS_PER_STEP &&
      (now - lastEncoderStepMs) >= ENCODER_STEP_MIN_INTERVAL_MS) {
    d = (int)(encRemainder / ENCODER_COUNTS_PER_STEP);
    encRemainder -= (long)d * ENCODER_COUNTS_PER_STEP;
    lastEncoderStepMs = now;
  }

  if (d != 0) {
    if (currentMenu == MAIN_SCR && !isAutoMode) {
      // MAN: target ustawiany lokalnie enkoderem.
      // Prędkość zmiany targetu zależy od szybkości kręcenia enkodera.
      unsigned long dt = now - lastEncoderMove;
      int stepScale = 1;
      if (dt <= 40) stepScale = 10;
      else if (dt <= 80) stepScale = 5;
      else if (dt <= 150) stepScale = 2;
      int dScaled = (d > 0) ? stepScale : -stepScale;

      // Zmieniaj tylko AZ (EL zostawiamy do rozbudowy/komendy z aplikacji)
      targetAZ = constrain(targetAZ + dScaled, 0, PAN_MAX);
      lastEncoderMove = now;
    } else if (currentMenu == MENU_ROOT) {
      menuIdx = constrain(menuIdx + d, 0, 6);
    } else if (currentMenu == ALARMS_MENU) {
      // Policz aktywne alarmy
      int activeCount = 0;
      for (int i = 0; i < MAX_ALARMS - 1; i++) {
        if (alarms[i].active) activeCount++;
      }
      alarmMenuIdx = constrain(alarmMenuIdx + d, 0, max(0, activeCount - 1));
    } else if (currentMenu == ROTOR_SETTINGS) {
      rotorMenuIdx = constrain(rotorMenuIdx + d, 0, 5);
    } else if (currentMenu == SET_PAN) {
      PAN_MAX = constrain(PAN_MAX + d, 0, 365);
    } else if (currentMenu == SET_TILT) {
      TILT_MAX = constrain(TILT_MAX + d, 0, 90);
    } else if (currentMenu == SET_COMM_SPEED) {
      // Przełącz między 2400 a 9600 (jedno kliknięcie = zmiana)
      commSpeed = (commSpeed == 2400) ? 9600 : 2400;
    } else if (currentMenu == SET_DIRECTION) {
      direction = constrain(direction + d, 0, 359);
    } else if (currentMenu == SET_PAN_DIR) {
      if (d != 0) reversePAN = !reversePAN;
    } else if (currentMenu == SET_TILT_DIR) {
      if (d != 0) reverseTILT = !reverseTILT;
    } else if (currentMenu == REMOTE_CTRL) {
      remoteMenuIdx = constrain(remoteMenuIdx + d, 0, 1);
    }
  }
}

void renderDisplay() {
  // Wyświetlanie
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Powiadomienie (jednorazowe) – pokazuj jako pełnoekranowe
  if (notificationActive) {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print("!");
    display.setTextSize(1);
    display.setCursor(12, 0);
    display.print("ALARM");
    display.setCursor(0, 16);
    display.print(notificationMessage);
    display.setCursor(0, 56);
    display.print("Kliknij aby zamknac");
  display.display();
    delay(10);
    return;
  }

  if (currentMenu == MAIN_SCR) {
    // --- Ekran główny ---
    // Góra: mała czcionka AUTO/MANUAL
    display.setCursor(0, 0);
    display.print(isAutoMode ? "AUTO" : "MANUAL");

    // Prawy górny róg: aktualna elewacja (CUR) - WIĘKSZA CZCIONKA
    char elBuf[12];
    int userEL = getUserElevation();
    snprintf(elBuf, sizeof(elBuf), "EL:%d", userEL);
    display.setTextSize(2);
    int elX = 128 - ((int)strlen(elBuf) * 12); // 12 px na znak przy setTextSize(2)
    if (elX < 0) elX = 0;
    display.setCursor(elX, 0); 
    display.print(elBuf);
    display.setTextSize(1);

    // Sprawdź czy są aktywne alarmy (np. brak kalibracji)
    bool anyAlarm = false;
    for (int i = 0; i < MAX_ALARMS - 1; i++) {
      if (alarms[i].active) { anyAlarm = true; break; }
    }

    // Środek: duża czcionka, tylko liczby TGT/CUR
    int curGeoAZ = getGeographicAzimuth();
    int tgtGeoAZ = internalToGeographic(targetAZ);
    display.setTextSize(3);
    display.setCursor(0, 22);
    display.printf("%03d/%03d", tgtGeoAZ, curGeoAZ);
    display.setTextSize(1);

    // Dół: MENU> (+ ALARM! jeśli są alarmy)
    display.setCursor(0, 56);
    display.print("MENU>");
    if (anyAlarm) {
      display.setCursor(68, 56);
      display.print("ALARM!");
    }
  } else {
    // --- Ekrany menu / ustawień (bez nakładania na ekran główny) ---
    display.setCursor(0, 0);
    if (currentMenu == INFO_SCR) display.print("INFO:");
    else if (currentMenu == ALARMS_MENU) display.print("ALARMY:");
    else if (currentMenu == CALIB_AZ_CONFIRM) display.print("KALIB AZ");
    else if (currentMenu == CALIB_EL_CONFIRM) display.print("KALIB EL");
    else display.print("MENU");

    if (currentMenu == MENU_ROOT) {
      const char* menuItems[] = {"AUTO/MANUAL", "ROTOR SET", "KALIB AZ", "KALIB EL", "ALARMY", "INFO", "REMOTE CTRL"};
      display.setCursor(0, 18);
      display.print(">");
      display.setCursor(10, 18);
      display.print(menuItems[menuIdx]);
      if (menuIdx == 0) {
        // Pod spodem pokaż aktualny tryb większą czcionką
        display.setCursor(0, 40);
        display.setTextSize(2);
        display.print(isAutoMode ? "AUTO" : "MANUAL");
        display.setTextSize(1);
      }
    } else if (currentMenu == ROTOR_SETTINGS) {
      const char* rotorItems[] = {"PAN", "TILT", "SPEED", "DIR", "PAN SCALE", "TILT SCALE"};
      display.setCursor(0, 18);
      display.print("ROTOR SET:");
      display.setCursor(0, 32);
      display.print(">");
      display.setCursor(10, 32);
      display.print(rotorItems[rotorMenuIdx]);
      display.setCursor(0, 48);
      if (rotorMenuIdx == 0) display.printf("PAN=%03d", PAN_MAX);
      else if (rotorMenuIdx == 1) display.printf("TILT=%02d", TILT_MAX);
      else if (rotorMenuIdx == 2) display.printf("SPD=%d", commSpeed);
      else if (rotorMenuIdx == 3) display.printf("DIR=%03d", direction);
      else if (rotorMenuIdx == 4) display.printf("PAN:%s", reversePAN ? "REV" : "NORM");
      else if (rotorMenuIdx == 5) display.printf("TILT:%s", reverseTILT ? "REV" : "NORM");
    } else if (currentMenu == SET_PAN) {
      display.setCursor(0, 18);
      display.print("PAN MAX:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.printf("%03d", PAN_MAX);
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == SET_TILT) {
      display.setCursor(0, 18);
      display.print("TILT MAX:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.printf("%02d", TILT_MAX);
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == SET_COMM_SPEED) {
      display.setCursor(0, 18);
      display.print("COMM SPEED:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.printf("%d", commSpeed);
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == SET_DIRECTION) {
      display.setCursor(0, 18);
      display.print("DIRECTION:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.printf("%03d", direction);
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == SET_PAN_DIR) {
      display.setCursor(0, 18);
      display.print("PAN SCALE:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.print(reversePAN ? "REVERSE" : "NORMAL");
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == SET_TILT_DIR) {
      display.setCursor(0, 18);
      display.print("TILT SCALE:");
      display.setCursor(0, 32);
      display.setTextSize(2);
      display.print(reverseTILT ? "REVERSE" : "NORMAL");
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("Kliknij OK");
    } else if (currentMenu == ALARMS_MENU) {
      // Nagłówek jest już na górze: ALARMY:
      // pokaż wybrany aktywny alarm
      int idx = 0;
      bool any = false;
      for (int i = 0; i < MAX_ALARMS - 1; i++) {
        if (!alarms[i].active) continue;
        any = true;
        if (idx == alarmMenuIdx) {
          display.setCursor(0, 18);
          display.print(">");
          display.setCursor(10, 18);
          display.print(alarms[i].message);
          break;
        }
        idx++;
      }
      if (!any) {
        display.setCursor(0, 18);
        display.print("Brak alarmow");
      }
      display.setCursor(0, 56);
      display.print("Hold=Powrot");
    } else if (currentMenu == CALIB_AZ_CONFIRM) {
      // Bez "MENU" u góry – tylko instrukcja
      display.setCursor(0, 22);
      display.print("Aby rozpoczac");
      display.setCursor(0, 34);
      display.print("kalibracje nacisnij");
      display.setCursor(0, 46);
      display.print("przycisk");
      display.setCursor(0, 56);
      display.print("Hold=Powrot");
    } else if (currentMenu == CALIB_EL_CONFIRM) {
      display.setCursor(0, 22);
      display.print("Aby rozpoczac");
      display.setCursor(0, 34);
      display.print("kalibracje nacisnij");
      display.setCursor(0, 46);
      display.print("przycisk");
      display.setCursor(0, 56);
      display.print("Hold=Powrot");
    } else if (currentMenu == INFO_SCR) {
      // Nagłówek jest już na górze: INFO:
      // Odstępy między wierszami: 11 px
      display.setCursor(0, 12);
      display.printf("TR_AZ: %.2f", TR1D_AZ);
      display.setCursor(0, 23);
      display.printf("TR_EL: %.2f", TR1D_EL);
      display.setCursor(0, 34);
      display.printf("PAN:%d TILT:%d", PAN_MAX, TILT_MAX);
      // SPEED/DIR/kalibracja
      display.setCursor(0, 45);
      display.printf("SPD:%d DIR:%03d%s", commSpeed, direction, isCalibrated ? "" : " !");
      // IP na dole
      String ipStr = wifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
      display.setCursor(0, 55);
      display.print("IP: ");
      display.print(ipStr);
    } else if (currentMenu == REMOTE_CTRL) {
      const char* remoteItems[] = {"USBSERIAL", "TCPIP"};
      display.setCursor(0, 18);
      display.print("REMOTE CTRL:");
      display.setCursor(0, 32);
      display.print(">");
      display.setCursor(10, 32);
      display.print(remoteItems[remoteMenuIdx]);
      display.setCursor(0, 48);
      display.print("Kliknij wybierz");
      display.setCursor(0, 56);
      display.print("Hold=Powrot");
    }
  }
  
  display.display();
}
