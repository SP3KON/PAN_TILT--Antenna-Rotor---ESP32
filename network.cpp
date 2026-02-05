#include "network.h"
#include "webserver.h"
#include <Wire.h>

void scanI2C() {
  Serial.println("\n=== SKANOWANIE I2C ===");
  byte found = 0;
  
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("Znaleziono urzadzenie I2C na adresie: 0x%02X", address);
      
      // Rozpoznaj typ urządzenia
      if (address == 0x3C || address == 0x3D) {
        Serial.print(" (prawdopodobnie OLED SSD1306)");
      }
      
  Serial.println();
      found++;
    } else if (error == 4) {
      Serial.printf("Blad na adresie 0x%02X (nieznany blad)\n", address);
    }
  }
  
  if (found == 0) {
    Serial.println("Nie znaleziono zadnych urzaden I2C!");
  } else {
    Serial.printf("Znaleziono %d urzaden(ia) I2C\n", found);
  }
  Serial.println("=====================\n");
}

void initWiFi() {
  // Wczytaj SSID i hasło z Preferences
  String ssid = prefs.getString("wifiSSID", "");
  String password = prefs.getString("wifiPassword", "");
  bool silent = (remoteCtrlMode == REMOTE_USBSERIAL);
  
  if (ssid.length() > 0) {
    // Próbuj połączyć się z siecią domową
    if (!silent) Serial.printf("Laczenie z siecia: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // 10 sekund timeout
      delay(500);
      if (!silent) Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      if (!silent) Serial.printf("\nPolaczono z siecia! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      if (!silent) Serial.println("\nNie udalo sie polaczyc z siecia domowa - tworze AP");
    }
  }
  
  // Jeśli nie połączono z siecią domową, utwórz AP
  if (!wifiConnected) {
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP("ROTORSP3KON", "")) {  // Bez hasła
      wifiConnected = false;  // AP mode, nie "connected"
      if (!silent) Serial.printf("WiFi AP: ROTORSP3KON, IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
      if (!silent) Serial.println("BLAD: Nie mozna uruchomic WiFi AP!");
    }
  }
  
  // Uruchom serwery
  tcpServer.begin();
  setupWebServer();
  webServer.begin();
  if (!silent) {
    Serial.printf("TCP Server na porcie: %d\n", TCP_PORT);
    Serial.printf("Web Server na porcie: %d\n", HTTP_PORT);
  }
}
