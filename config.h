#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <Preferences.h>

// Port TCP dla komunikacji GS-232A (standardowy dla rotctld/Hamlib)
#define TCP_PORT 4533
// Port HTTP dla strony WWW
#define HTTP_PORT 80

// --- BEZPIECZNA KONFIGURACJA PINÓW ---
extern const int PIN_LED;    // Wbudowana LED ESP32 WROOM (GPIO 2)  a docelowo dodać na innym pinie dla kontroli pracy rotora 
extern const int PIN_ENC_SW; // Przycisk enkodera (z pull-up)
extern const int PIN_ENC_A;  // Enkoder A (z pull-up)
extern const int PIN_ENC_B;  // Enkoder B (z pull-up)  
extern const int RS485_RX;   
extern const int RS485_TX;   
extern const int I2C_SDA;    
extern const int I2C_SCL;    

// --- Debounce / krok enkodera ---
// ESP32Encoder (PCNT) potrafi liczyć kilka "countów" na jeden klik (detent) i może zbierać drgania.
// Dla typowego enkodera mechanicznego przy attachHalfQuad zwykle wychodzi ~2 county / 1 klik.
extern const int ENCODER_COUNTS_PER_STEP;         // ile countów = 1 krok w menu (dostosuj jeśli trzeba: 1/2/4)
extern const uint16_t ENCODER_STEP_MIN_INTERVAL_MS; // minimalny odstęp czasowy między krokami (filtr drgań)
extern const uint16_t BTN_DEBOUNCE_MS;           // debounce przycisku enkodera (SW)

// --- Obiekty (extern declarations) ---
extern WiFiServer tcpServer;
extern WiFiClient tcpClient;
extern WebServer webServer;
extern Adafruit_SSD1306 display;
extern ESP32Encoder encoder;
extern Preferences prefs;

// Tryby komunikacji zdalnej
enum RemoteCtrlMode {
  REMOTE_NONE = 0,
  REMOTE_USBSERIAL = 1,
  REMOTE_TCPIP = 2
};

enum MenuState { 
  MAIN_SCR, MENU_ROOT, ROTOR_SETTINGS, 
  SET_PAN, SET_TILT, SET_COMM_SPEED, SET_DIRECTION,
  SET_PAN_DIR, SET_TILT_DIR,
  CALIB_AZ_CONFIRM, CALIB_EL_CONFIRM,
  CALIB_AZ_LEFT, CALIB_AZ_RIGHT, CALIB_EL_DOWN, CALIB_EL_UP,
  INFO_SCR, ALARMS_MENU,
  REMOTE_CTRL, REMOTE_CTRL_SELECT
};

enum AlarmType {
  ALARM_NONE = 0,
  ALARM_NO_CALIBRATION = 1,
  ALARM_MOTOR_ERROR = 2,
  ALARM_COMM_ERROR = 3
};

// Mechanizm zabezpieczający przed oscylacjami przy osiąganiu celu
struct OscillationProtection {
  int attemptCount;        // Liczba prób osiągnięcia celu
  int lastDirection;       // Ostatni kierunek ruchu (-1, 0, 1)
  int bestPosition;        // Najlepsza pozycja osiągnięta (najbliższa celowi)
  int bestDiff;            // Najlepsza różnica od celu
  int lastTargetAZ;        // Ostatni cel AZ (do wykrywania zmiany celu)
  int lastTargetEL;        // Ostatni cel EL
};

// System alarmów
#define MAX_ALARMS 4
struct Alarm {
  AlarmType type;
  bool active;
  const char* message;
};

// --- Zmienne globalne (extern declarations) ---
extern bool isAutoMode;
extern RemoteCtrlMode remoteCtrlMode;
extern MenuState currentMenu;
extern int menuIdx;
extern int rotorMenuIdx;
extern int alarmMenuIdx;
extern int remoteMenuIdx;

// Parametry rotora
extern int PAN_MAX;
extern int TILT_MAX;
extern int commSpeed;
extern int direction;
extern bool reversePAN;
extern bool reverseTILT;

// Parametry WiFi
extern char wifiSSID[32];
extern char wifiPassword[64];
extern bool wifiConnected;

// Parametry kalibracji
extern float TR1D_AZ;
extern float TR1D_EL;
extern bool isCalibrated;

// Pozycje
extern int currentAZ;
extern int targetAZ;
extern int currentEL;
extern int targetEL;

// Śledzenie czasu obrotu
extern unsigned long motorStartTime;
extern bool motorRunning;
extern int motorDirection;
extern bool motorIsAZ;

// Oscillation protection
extern OscillationProtection oscProtectAZ;
extern OscillationProtection oscProtectEL;

// System alarmów
extern Alarm alarms[MAX_ALARMS];

// Powiadomienia na wyświetlaczu
extern bool notificationActive;
extern const char* notificationMessage;
extern unsigned long notificationStartTime;
extern unsigned long lastAutoNotificationTime;

// Pelco-D commands
extern byte pStop[7];
extern byte pRight[7];
extern byte pLeft[7];
extern byte pUp[7];
extern byte pDown[7];
extern byte pQueryPan[7];

// Communication check
extern unsigned long lastCommCheck;
extern const unsigned long commCheckInterval;

#endif // CONFIG_H
