#include "config.h"

// --- Pin definitions ---
const int PIN_LED    = 2;   // Wbudowana LED ESP32 WROOM (GPIO 2)
const int PIN_ENC_SW = 27;  // Przycisk enkodera (z pull-up)
const int PIN_ENC_A  = 32;  // Enkoder A (z pull-up)
const int PIN_ENC_B  = 33;  // Enkoder B (z pull-up)  
const int RS485_RX   = 13;  
const int RS485_TX   = 14;  
const int I2C_SDA    = 21;  
const int I2C_SCL    = 22;  

// --- Encoder constants ---
const int ENCODER_COUNTS_PER_STEP = 2;
const uint16_t ENCODER_STEP_MIN_INTERVAL_MS = 2;
const uint16_t BTN_DEBOUNCE_MS = 30;

// --- Global objects ---
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;
WebServer webServer(HTTP_PORT);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP32Encoder encoder;
Preferences prefs;

// --- Global variables ---
bool isAutoMode = false;
RemoteCtrlMode remoteCtrlMode = REMOTE_NONE;
MenuState currentMenu = MAIN_SCR;
int menuIdx = 0;
int rotorMenuIdx = 0;
int alarmMenuIdx = 0;
int remoteMenuIdx = 0;

// Parametry rotora
int PAN_MAX = 365;
int TILT_MAX = 90;
int commSpeed = 2400;
int direction = 0;
bool reversePAN = false;
bool reverseTILT = false;

// Parametry WiFi
char wifiSSID[32] = "";
char wifiPassword[64] = "";
bool wifiConnected = false;

// Parametry kalibracji
float TR1D_AZ = 0.0;
float TR1D_EL = 0.0;
bool isCalibrated = false;

// Pozycje
int currentAZ = 0;
int targetAZ = 0;
int currentEL = 0;
int targetEL = 0;

// Śledzenie czasu obrotu
unsigned long motorStartTime = 0;
bool motorRunning = false;
int motorDirection = 0;
bool motorIsAZ = true;

// Oscillation protection
OscillationProtection oscProtectAZ = {0, 0, 0, 9999, -1, -1};
OscillationProtection oscProtectEL = {0, 0, 0, 9999, -1, -1};

// System alarmów
Alarm alarms[MAX_ALARMS] = {
  {ALARM_NO_CALIBRATION, false, "BRAK KALIBRACJI"},
  {ALARM_MOTOR_ERROR, false, "BLAD SILNIKA"},
  {ALARM_COMM_ERROR, false, "BLAD KOMUNIKACJI"},
  {ALARM_NONE, false, ""}
};

// Powiadomienia na wyświetlaczu
bool notificationActive = false;
const char* notificationMessage = "";
unsigned long notificationStartTime = 0;
unsigned long lastAutoNotificationTime = 0;

// Pelco-D commands (Adres 1)
byte pStop[]     = {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01};
byte pRight[]    = {0xFF, 0x01, 0x00, 0x02, 0x20, 0x00, 0x23};
byte pLeft[]     = {0xFF, 0x01, 0x00, 0x04, 0x20, 0x00, 0x25};
byte pUp[]       = {0xFF, 0x01, 0x00, 0x08, 0x00, 0x20, 0x29};
byte pDown[]     = {0xFF, 0x01, 0x00, 0x10, 0x00, 0x20, 0x31};
byte pQueryPan[] = {0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52};

// Communication check
unsigned long lastCommCheck = 0;
const unsigned long commCheckInterval = 5000;
