/* * UNIWERSALNY STEROWNIK ROTORA SP3KON v3.2
 * Kompatybilność: WROVER-E, WROOM-32, DevKitC
 * System oparty na czasie obrotu (bez kompasu)
 * 
 * SYSTEM AZYMUTU:
 * - Wewnętrzna pozycja: 0 do PAN_MAX (ustawiany w menu ROTOR SETTINGS, domyślnie 0-365°)
 * - Azymut geograficzny: 0-359° (0° = północ, 90° = wschód, 180° = południe, 270° = zachód)
 * - Direction: ustawiany w menu ROTOR SETTINGS - wskazuje jaki kierunek geograficzny odpowiada pozycji 0
 * - Przykład: jeśli pozycja 0 jest skierowana na południe, ustaw Direction = 180°
 * 
 * PARAMETRY ROTORA (menu ROTOR SETTINGS):
 * - PAN: maksymalny kąt obrotu poziomego (0-365°)
 * - TILT: maksymalny kąt elewacji (0-90°)
 * - Communication Speed: prędkość komunikacji RS485 (2400 lub 9600 bps)
 * - Direction: kierunek geograficzny pozycji 0 (0-359°)
 * 
 * UWAGI DLA ESP32 WROOM:
 * - Enkoder na pinach 32/33 z wbudowanymi pull-upami
 * - Serial2 na pinach 13/14 jest bezpieczny
 * - WiFi: próbuje połączyć z siecią domową, jeśli nie może - tworzy AP
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <Preferences.h>

#include "config.h"
#include "globals.cpp"
#include "alarms.h"
#include "motor.h"
#include "position.h"
#include "calibration.h"
#include "rotctld.h"
#include "webserver.h"
#include "network.h"
#include "display_menu.h"


void setup() {
  Serial.begin(57600);
  delay(1000);  // Czekaj na stabilizację Serial
  
  prefs.begin("rotor", false);
  PAN_MAX = prefs.getInt("PAN_MAX", 365);
  TILT_MAX = prefs.getInt("TILT_MAX", 90);
  commSpeed = prefs.getInt("commSpeed", 2400);
  direction = prefs.getInt("direction", 0);
  remoteCtrlMode = (RemoteCtrlMode)prefs.getInt("remoteCtrlMode", REMOTE_USBSERIAL);
  isAutoMode = prefs.getBool("isAutoMode", false);
  reversePAN = prefs.getBool("reversePAN", false);
  reverseTILT = prefs.getBool("reverseTILT", false);

  // Synchronizuj indeks menu
  if (remoteCtrlMode == REMOTE_USBSERIAL) remoteMenuIdx = 0;
  else if (remoteCtrlMode == REMOTE_TCPIP) remoteMenuIdx = 1;

  bool silent = (remoteCtrlMode == REMOTE_USBSERIAL);

  // Inicjalizuj I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!silent) scanI2C();
  
  // Inicjalizuj Serial2 z wczytaną prędkością
  Serial2.begin(commSpeed, SERIAL_8N1, RS485_RX, RS485_TX);
  
  // Wczytaj parametry kalibracji
  TR1D_AZ = prefs.getFloat("TR1D_AZ", 0.0);
  TR1D_EL = prefs.getFloat("TR1D_EL", 0.0);
  currentAZ = prefs.getInt("currentAZ", 0);
  currentEL = prefs.getInt("currentEL", 0);
  
  // Ogranicz pozycje do aktualnych zakresów
  currentAZ = constrain(currentAZ, 0, PAN_MAX);
  currentEL = constrain(currentEL, 0, TILT_MAX);
  
  isCalibrated = (TR1D_AZ > 0 && TR1D_EL > 0);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);

  // Inicjalizacja OLED
  if (!silent) Serial.println("Inicjalizacja OLED...");
  delay(200);  // Dłuższe opóźnienie - ekran potrzebuje czasu na rozruch
  
  // Wyczyść stan I2C przed inicjalizacją
  Wire.beginTransmission(0x3C);
  Wire.endTransmission();
  delay(50);
  
  // Spróbuj najpierw z SSD1306_SWITCHCAPVCC (domyślne dla większości modułów z wewnętrznym boost)
  bool displayOk = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  // Jeśli nie zadziałało, spróbuj z SSD1306_EXTERNALVCC (dla modułów bez boostera)
  if (!displayOk) {
    if (!silent) Serial.println("Proba z SSD1306_EXTERNALVCC...");
    delay(100);
    displayOk = display.begin(SSD1306_EXTERNALVCC, 0x3C);
  }
  
  if (displayOk) {
    if (!silent) Serial.println("OLED zainicjalizowany poprawnie!");
    // Poprawna sekwencja inicjalizacji dla Adafruit SSD1306
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ESP32 ROTOR");
    display.display();  // WYMAGANE - wyświetl zawartość bufora
    delay(100);
    if (!silent) Serial.println("Test wyświetlania wykonany.");
  } else {
    if (!silent) Serial.println("OLED NIE DZIAŁA - program kontynuuje bez wyświetlacza.");
  }
  

  // Inicjalizacja WiFi - zawsze próbuje połączyć z siecią domową, potem AP
  initWiFi();

  encoder.attachHalfQuad(PIN_ENC_A, PIN_ENC_B);
  encoder.clearCount();
  
  if (!silent) {
    Serial.printf("TR1D_AZ: %.2f ms/st, TR1D_EL: %.2f ms/st\n", TR1D_AZ, TR1D_EL);
    Serial.printf("Pozycja startowa: AZ=%d, EL=%d\n", currentAZ, currentEL);
  }

  // Wykonaj bazowanie po uruchomieniu
  performHoming();
}

void loop() {
  checkAlarms();    // Sprawdź alarmy logiczne
  // checkRotorComm(); // Wyłączone - rotor CCTV nie obsługuje odpowiedzi (brak nadajnika)
  
  // Obsługa powiadomień (jednorazowe, znika po kliknięciu)
  if (notificationActive) {
    if (digitalRead(PIN_ENC_SW) == LOW) {
      delay(BTN_DEBOUNCE_MS);
      if (digitalRead(PIN_ENC_SW) == LOW) {
        while (digitalRead(PIN_ENC_SW) == LOW) delay(10);
        notificationActive = false;  // Usuń powiadomienie po kliknięciu
      }
    }
    // Powiadomienie znika też automatycznie po 5 sekundach
    if (millis() - notificationStartTime > 5000) {
      notificationActive = false;
    }
  }
  
  // Obsługa WebServer (strona WWW z ustawieniami)
  webServer.handleClient();
  
  updatePosition();  // Aktualizuj pozycję na podstawie czasu
  
  // Obsługa komend GS-232A w zależności od trybu komunikacji
  
  // 1. USBSERIAL - komendy przez Serial USB (bez logów)
  if (remoteCtrlMode == REMOTE_USBSERIAL) {
    static char usbCmdBuf[16];
    static uint8_t usbCmdLen = 0;
    static unsigned long lastUsbByteMs = 0;
    bool gotUsbData = false;

    while (Serial.available()) {
      char c = (char)Serial.read();
      gotUsbData = true;
      lastUsbByteMs = millis();
      if (c == '\r' || c == '\n') {
        if (usbCmdLen > 0) {
          usbCmdBuf[usbCmdLen] = '\0';
          handleGs232Command(usbCmdBuf, usbCmdLen, &Serial, false); // false = bez logów
          usbCmdLen = 0;
        }
      } else {
        if (usbCmdLen < sizeof(usbCmdBuf) - 1) {
          usbCmdBuf[usbCmdLen++] = c;
        }
      }
    }
    // Jeśli brak terminatora, a minął krótki czas - potraktuj bufor jako komendę
    if (!gotUsbData && usbCmdLen > 0 && (millis() - lastUsbByteMs) > 50) {
      usbCmdBuf[usbCmdLen] = '\0';
      handleGs232Command(usbCmdBuf, usbCmdLen, &Serial, false);
      usbCmdLen = 0;
    }
  }
  
  // 2. TCPIP - komendy przez WiFi TCP (port 4533)
  if (remoteCtrlMode == REMOTE_TCPIP) {
    // Sprawdź czy jest nowe połączenie (nawet jeśli stare jeszcze "wisi")
    WiFiClient newClient = tcpServer.available();
    if (newClient) {
      if (tcpClient && tcpClient.connected()) {
        tcpClient.stop(); // Zamknij stare połączenie, jeśli przyszło nowe
      }
      tcpClient = newClient;
      tcpClient.setNoDelay(true); // Wyłącz algorytm Nagle'a dla szybszej reakcji
      Serial.println("[TCP] Nowy klient polaczony");
    }
    
    // Obsługa danych od klienta TCP
    if (tcpClient && tcpClient.connected()) {
      static char tcpCmdBuf[64];
      static uint8_t tcpCmdLen = 0;
      static unsigned long lastTcpByteMs = 0;
      
      while (tcpClient.available()) {
        char c = (char)tcpClient.read();
        lastTcpByteMs = millis();
        if (c == '\r' || c == '\n') {
          if (tcpCmdLen > 0) {
            tcpCmdBuf[tcpCmdLen] = '\0';
            handleRotctldCommand(tcpCmdBuf, tcpCmdLen, &tcpClient, true);
            tcpCmdLen = 0;
          }
        } else {
          if (tcpCmdLen < sizeof(tcpCmdBuf) - 1) {
            tcpCmdBuf[tcpCmdLen++] = c;
          }
        }
      }
      
      // Timeout dla komend bez terminatora
      if (tcpCmdLen > 0 && (millis() - lastTcpByteMs) > 100) {
        tcpCmdBuf[tcpCmdLen] = '\0';
        handleRotctldCommand(tcpCmdBuf, tcpCmdLen, &tcpClient, true);
        tcpCmdLen = 0;
      }
    }
  }
  
  handleEncoderAndButton();  // From display_menu module
  autoPositionControl();     // From motor module
  renderDisplay();           // From display_menu module
  
  delay(10);
}
