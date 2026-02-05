#include "calibration.h"
#include "motor.h"
#include "position.h"

void calibrateAZ() {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("KALIB AZ: LEWO");
  display.setCursor(0, 35);
  display.print("Kliknij gdy MAX");
  display.display();
  
  startMotor(-1, true);  // Obracaj w lewo
  
  // Czekaj na kliknięcie
  while (digitalRead(PIN_ENC_SW) == HIGH) {
    delay(10);
  }
  while (digitalRead(PIN_ENC_SW) == LOW) {
    delay(10);
  }
  
  stopMotor();
  unsigned long timeLeft = millis();
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("KALIB AZ: PRAWO");
  display.setCursor(0, 35);
  display.print("Kliknij gdy MAX");
  display.display();
  
  startMotor(1, true);  // Obracaj w prawo
  
  // Czekaj na kliknięcie
  while (digitalRead(PIN_ENC_SW) == HIGH) {
    delay(10);
  }
  while (digitalRead(PIN_ENC_SW) == LOW) {
    delay(10);
  }
  
  unsigned long timeRight = millis();
  stopMotor();
  
  unsigned long totalTime = timeRight - timeLeft;
  TR1D_AZ = (float)totalTime / (float)PAN_MAX;  // Czas na 1 stopień
  
  prefs.putFloat("TR1D_AZ", TR1D_AZ);
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.printf("AZ OK: %.1f ms/st", TR1D_AZ);
  display.display();
  delay(2000);
  
  currentAZ = 0;  // Ustaw na pozycję 0 (MAXLEFT)
  prefs.putInt("currentAZ", currentAZ);
}

void calibrateEL() {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("KALIB EL: DOL");
  display.setCursor(0, 35);
  display.print("Kliknij gdy MAX");
  display.display();
  
  startMotor(-1, false);  // Obracaj w dół
  
  // Czekaj na kliknięcie
  while (digitalRead(PIN_ENC_SW) == HIGH) {
    delay(10);
  }
  while (digitalRead(PIN_ENC_SW) == LOW) {
    delay(10);
  }
  
  stopMotor();
  unsigned long timeDown = millis();
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("KALIB EL: GORA");
  display.setCursor(0, 35);
  display.print("Kliknij gdy MAX");
  display.display();
  
  startMotor(1, false);  // Obracaj w górę
  
  // Czekaj na kliknięcie
  while (digitalRead(PIN_ENC_SW) == HIGH) {
    delay(10);
  }
  while (digitalRead(PIN_ENC_SW) == LOW) {
    delay(10);
  }
  
  unsigned long timeUp = millis();
  stopMotor();
  
  unsigned long totalTime = timeUp - timeDown;
  TR1D_EL = (float)totalTime / (float)TILT_MAX;  // Czas na 1 stopień
  
  prefs.putFloat("TR1D_EL", TR1D_EL);
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.printf("EL OK: %.1f ms/st", TR1D_EL);
  display.display();
  delay(2000);
  
  currentEL = 0;  // Ustaw na pozycję 0
  prefs.putInt("currentEL", currentEL);
}

// Funkcja bazowania (homing) po uruchomieniu
void performHoming() {
  if (!isCalibrated) return;
  
  bool silent = (remoteCtrlMode == REMOTE_USBSERIAL);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("BAZOWANIE");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Kliknij aby pominac");
  display.setCursor(0, 45);
  display.print("Start za 2 sek...");
  display.display();

  // Krótkie oczekiwanie na pominięcie
  unsigned long waitStart = millis();
  while (millis() - waitStart < 2000) {
    if (digitalRead(PIN_ENC_SW) == LOW) {
      if (!silent) Serial.println("[HOMING] Pominieto bazowanie.");
      return;
    }
    delay(10);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("BAZOWANIE");
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("AZ -> 0...");
  display.display();

  // 1. AZYMUT (W LEWO do oporu)
  unsigned long moveTimeAZ = (unsigned long)(PAN_MAX * TR1D_AZ);
  if (moveTimeAZ > 0) {
    if (!silent) Serial.printf("[HOMING] AZ: %lu ms\n", moveTimeAZ);
    startMotor(-1, true);
    unsigned long start = millis();
    while (millis() - start < moveTimeAZ) {
      if (digitalRead(PIN_ENC_SW) == LOW) { stopMotor(); return; } // Przerwij
      webServer.handleClient(); 
      delay(10);
    }
    stopMotor();
  }

  display.setCursor(0, 45);
  display.print("EL -> 0...");
  display.display();

  // 2. ELEWACJA (W DÓŁ do oporu)
  unsigned long moveTimeEL = (unsigned long)(TILT_MAX * TR1D_EL);
  if (moveTimeEL > 0) {
    if (!silent) Serial.printf("[HOMING] EL: %lu ms\n", moveTimeEL);
    startMotor(-1, false);
    unsigned long start = millis();
    while (millis() - start < moveTimeEL) {
      if (digitalRead(PIN_ENC_SW) == LOW) { stopMotor(); return; } // Przerwij
      webServer.handleClient();
      delay(10);
    }
    stopMotor();
  }

  // Reset pozycji
  currentAZ = 0;
  targetAZ = 0;
  currentEL = 0;
  prefs.putInt("currentAZ", 0);
  prefs.putInt("currentEL", 0);
  
  // Ustaw cel na 0 użytkownika (wypoziomowanie)
  targetEL = userToInternalEL(0);
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.print("GOTOWE");
  display.display();
  delay(1000);
}
