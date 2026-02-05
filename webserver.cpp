#include "webserver.h"
#include "position.h"
#include "motor.h"

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>SP3KON Rotor Control</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:10px;background:#f0f2f5;text-align:center;}";
  html += "h1{color:#1a73e8;margin-bottom:10px;} .section{margin:15px auto;padding:15px;background:white;border-radius:10px;box-shadow:0 2px 4px rgba(0,0,0,0.1);max-width:500px;}";
  html += ".btn{padding:12px 24px;border:none;border-radius:5px;cursor:pointer;font-weight:bold;margin:5px;transition:0.3s;}";
  html += ".btn-auto{background:#1a73e8;color:white;} .btn-man{background:#ea4335;color:white;}";
  html += ".btn-ctrl{background:#5f6368;color:white;width:60px;height:60px;font-size:24px;} .btn-stop{background:#ea4335;width:130px;}";
  html += ".btn-set{background:#34a853;color:white;} .control-grid{display:grid;grid-template-columns:repeat(3,70px);grid-gap:10px;justify-content:center;margin:15px 0;}";
  html += "input{padding:10px;border:1px solid #ddd;border-radius:5px;width:70px;margin:5px;} label{font-weight:bold;display:block;margin-top:10px;}";
  html += "a{color:#1a73e8;text-decoration:none;font-weight:bold;}.info-link{display:block;margin:10px 0;font-size:18px;}</style></head><body>";
  
  html += "<h1>SP3KON Rotor Control</h1>";
  html += "<a href='/info' class='info-link'>📄 INSTRUKCJA (INFO)</a>";

  // --- SEKCOJA STEROWANIA ---
  html += "<div class='section'><h2>Sterowanie</h2>";
  
  // Pozycje na żywo (aktualizowane przez JavaScript)
  html += "<div style='margin-bottom:15px;padding:10px;background:#e8f0fe;border-radius:5px;'>";
  html += "<p style='margin:5px 0;'><strong>Pozycja AZ:</strong> <span id='positionAZ'>" + String(getGeographicAzimuth()) + "</span>°</p>";
  html += "<p style='margin:5px 0;'><strong>Pozycja EL:</strong> <span id='positionEL'>" + String(getUserElevation()) + "</span>°</p>";
  html += "</div>";
  
  // Przełącznik AUTO/MANUAL
  html += "<p>Tryb pracy: <strong>" + String(isAutoMode ? "AUTO (Zdalny)" : "MANUAL (Ręczny)") + "</strong></p>";
  html += "<form action='/toggleAuto' method='POST'><button class='btn " + String(isAutoMode ? "btn-man" : "btn-auto") + "'>";
  html += isAutoMode ? "PRZEŁĄCZ NA RĘCZNY" : "PRZEŁĄCZ NA AUTO";
  html += "</button></form>";

  if (!isAutoMode) {
    // Klawisze strzałek dla trybu MANUAL z obsługą przytrzymania
    html += "<div class='control-grid'>";
    html += "<div></div>";
    html += "<button class='btn btn-ctrl' id='btnUp' type='button'>▲</button>";
    html += "<div></div>";
    html += "<button class='btn btn-ctrl' id='btnLeft' type='button'>◀</button>";
    html += "<button class='btn btn-ctrl btn-stop' id='btnStop' type='button' style='width:60px;'>■</button>";
    html += "<button class='btn btn-ctrl' id='btnRight' type='button'>▶</button>";
    html += "<div></div>";
    html += "<button class='btn btn-ctrl' id='btnDown' type='button'>▼</button>";
    html += "<div></div>";
    html += "</div>";
    html += "<p><small>Przytrzymaj strzałkę aby poruszać ciągle | Kliknij ■ aby zatrzymać</small></p>";
  } else {
    // Pole USTAW dla trybu AUTO
    html += "<form action='/setpos' method='GET' style='margin-top:20px;'>";
    html += "AZ: <input type='number' name='az' value='000' min='0' max='359'>";
    html += " EL: <input type='number' name='el' value='000' min='0' max='90'>";
    html += "<br><button class='btn btn-set'>USTAW POZYCJĘ</button></form>";
  }
  html += "</div>";

  // --- SEKCOJA USTAWIEŃ ROTORA ---
  html += "<div class='section'><h2>Ustawienia Rotora</h2><form action='/save' method='POST'>";
  html += "<label>Zakres PAN (0-365°):</label><input type='number' name='pan' value='" + String(PAN_MAX) + "' min='0' max='365' style='width:150px;'>";
  html += "<label>Zakres TILT (0-90°):</label><input type='number' name='tilt' value='" + String(TILT_MAX) + "' min='0' max='90' style='width:150px;'>";
  html += "<label>Prędkość RS485:</label><select name='speed' style='padding:10px;width:170px;'>";
  html += "<option value='2400'" + String(commSpeed == 2400 ? " selected" : "") + ">2400 bps</option>";
  html += "<option value='9600'" + String(commSpeed == 9600 ? " selected" : "") + ">9600 bps</option></select>";
  html += "<label>Kierunek 0 (Azymut):</label><input type='number' name='dir' value='" + String(direction) + "' min='0' max='359' style='width:150px;'>";
  html += "<label>Skala PAN:</label><select name='revpan' style='padding:10px;width:170px;'>";
  html += "<option value='0'" + String(!reversePAN ? " selected" : "") + ">Normal</option><option value='1'" + String(reversePAN ? " selected" : "") + ">Reverse</option></select>";
  html += "<label>Skala TILT:</label><select name='revtilt' style='padding:10px;width:170px;'>";
  html += "<option value='0'" + String(!reverseTILT ? " selected" : "") + ">Normal</option><option value='1'" + String(reverseTILT ? " selected" : "") + ">Reverse</option></select>";
  html += "<br><button class='btn btn-auto' style='margin-top:15px;'>ZAPISZ USTAWIENIA</button></form></div>";

  // --- KONFIGURACJA WIFI ---
  String savedSSID = prefs.getString("wifiSSID", "");
  html += "<div class='section'><h2>Konfiguracja WiFi</h2>";
  html += "<form action='/savewifi' method='POST'>";
  html += "<label>SSID:</label><input type='text' name='ssid' value='" + savedSSID + "' style='width:200px;'>";
  html += "<label>Hasło:</label><input type='password' name='pass' style='width:200px;'>";
  html += "<br><button class='btn btn-auto' style='margin-top:15px;'>ZAPISZ WIFI</button></form></div>";

  // --- INFORMACJE O SYSTEMIE ---
  html += "<div class='section'><h2>System Info</h2>";
  
  // Pobierz wartości dokładnie tak samo jak dla OLED
  int displayAZ = getGeographicAzimuth();
  int displayEL = getUserElevation();
  
  html += "<p><strong>Pozycja AZ:</strong> " + String(displayAZ) + "°</p>";
  html += "<p><strong>Pozycja EL:</strong> " + String(displayEL) + "°</p>";
  html += "<p><strong>Kalibracja:</strong> " + String(isCalibrated ? "TAK" : "BRAK") + "</p>";
  
  // WiFi Status w System Info
  String ipStr = wifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String modeStr = wifiConnected ? "Połączono (STA)" : "Punkt Dostępowy (AP)";
  html += "<hr><p><strong>Status WiFi:</strong> " + modeStr + "</p>";
  html += "<p><strong>Adres IP:</strong> " + ipStr + "</p>";
  
  if (isCalibrated) {
    html += "<p><small>TR1D_AZ: " + String(TR1D_AZ, 2) + " ms/°, TR1D_EL: " + String(TR1D_EL, 2) + " ms/°</small></p>";
  }
  html += "</div>";
  
  // JavaScript do aktualizacji pozycji na żywo i sterowania ręcznego (na końcu body)
  html += "<script>";
  html += "console.log('Skrypt załadowany');";
  html += "var manualInterval = null;";
  html += "var currentManualDir = null;";
  html += "";
  html += "function updatePosition() {";
  html += "  var azEl = document.getElementById('positionAZ');";
  html += "  var elEl = document.getElementById('positionEL');";
  html += "  if (!azEl || !elEl) {";
  html += "    console.log('Elementy DOM nie znalezione: positionAZ=' + (azEl ? 'OK' : 'NULL') + ', positionEL=' + (elEl ? 'OK' : 'NULL'));";
  html += "    return;";
  html += "  }";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/position', true);";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState === 4) {";
  html += "      if (xhr.status === 200) {";
  html += "        try {";
  html += "          var data = JSON.parse(xhr.responseText);";
  html += "          azEl.textContent = data.az;";
  html += "          elEl.textContent = data.el;";
  html += "        } catch(e) {";
  html += "          console.error('Błąd parsowania JSON:', e, xhr.responseText);";
  html += "        }";
  html += "      } else {";
  html += "        console.error('Błąd HTTP:', xhr.status);";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() { console.error('Błąd połączenia z /api/position'); };";
  html += "  xhr.send();";
  html += "}";
  html += "";
  html += "function manualControl(dir, start) {";
  html += "  console.log('manualControl:', dir, start);";
  html += "  if (dir === 'stop') {";
  html += "    if (manualInterval) {";
  html += "      clearInterval(manualInterval);";
  html += "      manualInterval = null;";
  html += "    }";
  html += "    currentManualDir = null;";
  html += "    sendManualCommand('stop');";
  html += "    return;";
  html += "  }";
  html += "  ";
  html += "  if (start) {";
  html += "    if (currentManualDir !== dir) {";
  html += "      if (manualInterval) {";
  html += "        clearInterval(manualInterval);";
  html += "      }";
  html += "      currentManualDir = dir;";
  html += "      sendManualCommand(dir);";
  html += "      manualInterval = setInterval(function() {";
  html += "        sendManualCommand(dir);";
  html += "      }, 250);";
  html += "    }";
  html += "  } else {";
  html += "    if (currentManualDir === dir) {";
  html += "      if (manualInterval) {";
  html += "        clearInterval(manualInterval);";
  html += "        manualInterval = null;";
  html += "      }";
  html += "      currentManualDir = null;";
  html += "      sendManualCommand('stop');";
  html += "    }";
  html += "  }";
  html += "}";
  html += "";
  html += "function sendManualCommand(dir) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/manual?dir=' + dir, true);";
  html += "  xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');";
  html += "  xhr.onerror = function() { console.error('Błąd wysyłania komendy:', dir); };";
  html += "  xhr.send();";
  html += "}";
  html += "";
  html += "function initManualButtons() {";
  html += "  console.log('Inicjalizacja przycisków');";
  html += "  var btnUp = document.getElementById('btnUp');";
  html += "  var btnDown = document.getElementById('btnDown');";
  html += "  var btnLeft = document.getElementById('btnLeft');";
  html += "  var btnRight = document.getElementById('btnRight');";
  html += "  var btnStop = document.getElementById('btnStop');";
  html += "  ";
  html += "  if (btnUp) {";
  html += "    btnUp.addEventListener('mousedown', function(e) { e.preventDefault(); manualControl('up', true); });";
  html += "    btnUp.addEventListener('mouseup', function(e) { e.preventDefault(); manualControl('up', false); });";
  html += "    btnUp.addEventListener('mouseleave', function(e) { e.preventDefault(); manualControl('up', false); });";
  html += "    btnUp.addEventListener('touchstart', function(e) { e.preventDefault(); manualControl('up', true); });";
  html += "    btnUp.addEventListener('touchend', function(e) { e.preventDefault(); manualControl('up', false); });";
  html += "    btnUp.addEventListener('touchcancel', function(e) { e.preventDefault(); manualControl('up', false); });";
  html += "  }";
  html += "  if (btnDown) {";
  html += "    btnDown.addEventListener('mousedown', function(e) { e.preventDefault(); manualControl('down', true); });";
  html += "    btnDown.addEventListener('mouseup', function(e) { e.preventDefault(); manualControl('down', false); });";
  html += "    btnDown.addEventListener('mouseleave', function(e) { e.preventDefault(); manualControl('down', false); });";
  html += "    btnDown.addEventListener('touchstart', function(e) { e.preventDefault(); manualControl('down', true); });";
  html += "    btnDown.addEventListener('touchend', function(e) { e.preventDefault(); manualControl('down', false); });";
  html += "    btnDown.addEventListener('touchcancel', function(e) { e.preventDefault(); manualControl('down', false); });";
  html += "  }";
  html += "  if (btnLeft) {";
  html += "    btnLeft.addEventListener('mousedown', function(e) { e.preventDefault(); manualControl('left', true); });";
  html += "    btnLeft.addEventListener('mouseup', function(e) { e.preventDefault(); manualControl('left', false); });";
  html += "    btnLeft.addEventListener('mouseleave', function(e) { e.preventDefault(); manualControl('left', false); });";
  html += "    btnLeft.addEventListener('touchstart', function(e) { e.preventDefault(); manualControl('left', true); });";
  html += "    btnLeft.addEventListener('touchend', function(e) { e.preventDefault(); manualControl('left', false); });";
  html += "    btnLeft.addEventListener('touchcancel', function(e) { e.preventDefault(); manualControl('left', false); });";
  html += "  }";
  html += "  if (btnRight) {";
  html += "    btnRight.addEventListener('mousedown', function(e) { e.preventDefault(); manualControl('right', true); });";
  html += "    btnRight.addEventListener('mouseup', function(e) { e.preventDefault(); manualControl('right', false); });";
  html += "    btnRight.addEventListener('mouseleave', function(e) { e.preventDefault(); manualControl('right', false); });";
  html += "    btnRight.addEventListener('touchstart', function(e) { e.preventDefault(); manualControl('right', true); });";
  html += "    btnRight.addEventListener('touchend', function(e) { e.preventDefault(); manualControl('right', false); });";
  html += "    btnRight.addEventListener('touchcancel', function(e) { e.preventDefault(); manualControl('right', false); });";
  html += "  }";
  html += "  if (btnStop) {";
  html += "    btnStop.addEventListener('click', function(e) { e.preventDefault(); manualControl('stop', false); });";
  html += "  }";
  html += "  console.log('Przyciski zainicjalizowane: Up=' + (btnUp ? 'OK' : 'NULL') + ', Down=' + (btnDown ? 'OK' : 'NULL') + ', Left=' + (btnLeft ? 'OK' : 'NULL') + ', Right=' + (btnRight ? 'OK' : 'NULL') + ', Stop=' + (btnStop ? 'OK' : 'NULL'));";
  html += "}";
  html += "";
  html += "// Inicjalizacja";
  html += "console.log('Rozpoczynam inicjalizację');";
  html += "updatePosition();"; // Pierwsze wywołanie
  html += "setInterval(updatePosition, 500);"; // Co 500ms
  html += "initManualButtons();"; // Inicjalizuj przyciski
  html += "";
  html += "window.addEventListener('beforeunload', function() {";
  html += "  if (manualInterval) {";
  html += "    sendManualCommand('stop');";
  html += "  }";
  html += "});";
  html += "</script>";
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleSaveSettings() {
  if (webServer.hasArg("pan")) {
    PAN_MAX = webServer.arg("pan").toInt();
    PAN_MAX = constrain(PAN_MAX, 0, 365);
    prefs.putInt("PAN_MAX", PAN_MAX);
  }
  if (webServer.hasArg("tilt")) {
    TILT_MAX = webServer.arg("tilt").toInt();
    TILT_MAX = constrain(TILT_MAX, 0, 90);
    prefs.putInt("TILT_MAX", TILT_MAX);
  }
  if (webServer.hasArg("speed")) {
    commSpeed = webServer.arg("speed").toInt();
    prefs.putInt("commSpeed", commSpeed);
    Serial2.end();
    delay(100);
    Serial2.begin(commSpeed, SERIAL_8N1, RS485_RX, RS485_TX);
  }
  if (webServer.hasArg("dir")) {
    direction = webServer.arg("dir").toInt();
    direction = constrain(direction, 0, 359);
    prefs.putInt("direction", direction);
  }
  if (webServer.hasArg("revpan")) {
    reversePAN = (webServer.arg("revpan") == "1");
    prefs.putBool("reversePAN", reversePAN);
  }
  if (webServer.hasArg("revtilt")) {
    reverseTILT = (webServer.arg("revtilt") == "1");
    prefs.putBool("reverseTILT", reverseTILT);
  }
  webServer.send(200, "text/html", "<html><body><h1>Settings Saved!</h1><p><a href='/'>Back</a></p></body></html>");
}

void handleToggleAuto() {
  isAutoMode = !isAutoMode;
  prefs.putBool("isAutoMode", isAutoMode);
  if (!isAutoMode) {
    stopMotor();
    targetAZ = currentAZ;
    targetEL = currentEL;
  }
  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

void handleManualCmd() {
  if (isAutoMode) {
    webServer.send(400, "text/plain", "Błąd: Najpierw wyłącz tryb AUTO");
    return;
  }
  
  String cmd = webServer.arg("dir");
  if (cmd == "left") startMotor(-1, true);
  else if (cmd == "right") startMotor(1, true);
  else if (cmd == "up") startMotor(1, false);
  else if (cmd == "down") startMotor(-1, false);
  else if (cmd == "stop") stopMotor();
  
  // Sprawdź czy to żądanie AJAX (z nagłówkiem X-Requested-With)
  String requestedWith = webServer.header("X-Requested-With");
  if (requestedWith == "XMLHttpRequest") {
    // Zwróć prostą odpowiedź JSON dla AJAX
    webServer.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    // Standardowe przekierowanie dla zwykłych żądań (kompatybilność wsteczna)
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  }
}

void handleSetPos() {
  if (!isAutoMode) {
    webServer.send(400, "text/plain", "Błąd: Najpierw włącz tryb AUTO");
    return;
  }

  if (webServer.hasArg("az")) {
    int azVal = webServer.arg("az").toInt();
    targetAZ = geographicToInternal(azVal % 360);
  }
  if (webServer.hasArg("el")) {
    int elVal = webServer.arg("el").toInt();
    targetEL = userToInternalEL(constrain(elVal, 0, 90));
  }
  
  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

void handlePositionAPI() {
  int displayAZ = getGeographicAzimuth();
  int displayEL = getUserElevation();
  
  String json = "{";
  json += "\"az\":" + String(displayAZ) + ",";
  json += "\"el\":" + String(displayEL);
  json += "}";
  
  webServer.send(200, "application/json", json);
}

void handleInfo() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>SP3KON Rotor - INFO</title>";
  html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0;line-height:1.6;}";
  html += "h1{color:#333;} h2{color:#555;margin-top:30px;border-bottom:2px solid #ccc;padding-bottom:5px;}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:30px;border-radius:5px;box-shadow:0 2px 5px rgba(0,0,0,0.1);}";
  html += "pre{background:#f5f5f5;padding:15px;border-radius:5px;overflow-x:auto;white-space:pre-wrap;font-size:14px;}";
  html += "a{color:#2196F3;text-decoration:none;} a:hover{text-decoration:underline;}";
  html += ".back-link{display:block;margin-top:20px;padding:10px;background:#4CAF50;color:white;text-align:center;border-radius:5px;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>SP3KON Rotor Control - Instrukcja Obsługi</h1>";
  html += "<a href='/' class='back-link'>← Powrót do strony głównej</a>";
  
  // Wczytaj zawartość info.txt - osadzona w kodzie
  html += "<pre>";
  html += readInfoFile();
  html += "</pre>";
  
  html += "<a href='/' class='back-link'>← Powrót do strony głównej</a>";
  html += "</div></body></html>";
  
  webServer.send(200, "text/html", html);
}

String readInfoFile() {
  // Zawartość pliku info.txt osadzona bezpośrednio w kodzie
  // Ze względu na długość, zwracamy pełną treść z pliku info.txt
  // UWAGA: Plik info.txt zawiera informacje o wersji 3.2 - część informacji może wymagać aktualizacji
  
  return 
    "================================================================================\n"
    "    UNIWERSALNY STEROWNIK ROTORA SP3KON v3.2 - INSTRUKCJA OBSŁUGI\n"
    "================================================================================\n\n"
    
    "1. PODŁĄCZENIE ELEKTRYCZNE MODUŁÓW\n"
    "================================================================================\n\n"
    
    "ESP32 WROOM-32:\n"
    "----------------\n"
    "VCC      -> 3.3V (zasilanie)\n"
    "GND      -> GND (masa)\n"
    "EN       -> 3.3V (lub przycisk reset - opcjonalnie)\n\n"
    
    "WYŚWIETLACZ OLED SSD1306 (128x64, I2C):\n"
    "----------------------------------------\n"
    "VCC      -> 3.3V\n"
    "GND      -> GND\n"
    "SDA      -> GPIO 21 (I2C Data)\n"
    "SCL      -> GPIO 22 (I2C Clock)\n"
    "Adres I2C: 0x3C\n\n"
    
    "ENKODER OBROTOWY Z PRZYCISKIEM:\n"
    "--------------------------------\n"
    "VCC      -> 3.3V\n"
    "GND      -> GND\n"
    "SW       -> GPIO 27 (Przycisk, z pull-up wbudowanym)\n"
    "A        -> GPIO 32 (Kanał A, z pull-up wbudowanym)\n"
    "B        -> GPIO 33 (Kanał B, z pull-up wbudowanym)\n\n"
    
    "MODUŁ RS485 (komunikacja z rotorem):\n"
    "-------------------------------------\n"
    "VCC      -> 5V (lub 3.3V w zależności od modułu)\n"
    "GND      -> GND\n"
    "RO (lub TX) -> GPIO 13 (ESP32 RX)\n"
    "DI (lub RX) -> GPIO 14 (ESP32 TX)\n"
    "  * Jeśli moduł ma piny RX/TX:\n"
    "    - Pin RX modułu -> GPIO 14 (TX ESP32)\n"
    "    - Pin TX modułu -> GPIO 13 (RX ESP32)\n"
    "DE/RE    -> Połączony z GND (tryb odbioru) lub sterowany osobno\n"
    "          (dla modułów z kontrolą kierunku)\n\n"
    
    "LED WSKAŹNIKOWY:\n"
    "-----------------\n"
    "Wbudowana LED ESP32 WROOM -> GPIO 2 (domyślnie używana w kodzie)\n\n"
    
    "UWAGI:\n"
    "------\n"
    "- Wszystkie moduły zasilane z 3.3V (oprócz RS485 jeśli wymaga 5V)\n"
    "- Wspólna masa (GND) dla wszystkich modułów\n"
    "- Piny 32 i 33 mają wbudowane pull-up, nie wymagają zewnętrznych rezystorów\n"
    "- Moduł RS485 może wymagać dodatkowego zasilania 5V w zależności od typu\n"
    "- Dla bezpieczeństwa użyj kondensatorów odsprzęgających (100µF) przy zasilaniu\n\n"
    
    "================================================================================\n\n"
    
    "2. DRZEWKO MENU\n"
    "================================================================================\n\n"
    
    "MAIN_SCR (Ekran główny)\n"
    "│\n"
    "├─> MENU_ROOT (Menu główne)\n"
    "│   │\n"
    "│   ├─> [0] AUTO/MANUAL - Przełączanie trybu pracy\n"
    "│   │   - kliknięcie: przełącza AUTO <-> MANUAL (bez wchodzenia do podmenu)\n"
    "│   │   - w trybie AUTO: rotorem steruje aplikacja nadrzędna (Serial USB lub TCP/IP)\n"
    "│   │   - w trybie MANUAL: cel ustawiany enkoderem\n"
    "│   │\n"
    "│   ├─> [1] ROTOR SET - Ustawienia rotora\n"
    "│   │   │\n"
    "│   │   ├─> [0] PAN - Maksymalny kąt obrotu (0-365°)\n"
    "│   │   ├─> [1] TILT - Maksymalny kąt elewacji (0-90°)\n"
    "│   │   ├─> [2] SPEED - Prędkość komunikacji RS485\n"
    "│   │   ├─> [3] DIR - Kierunek geograficzny pozycji 0 (0-359°)\n"
    "│   │   ├─> [4] PAN SCALE - Skala pozioma (Normal/Reverse)\n"
    "│   │   └─> [5] TILT SCALE - Skala pionowa (Normal/Reverse)\n"
    "│   │\n"
    "│   ├─> [2] KALIB AZ - Kalibracja azymutu\n"
    "│   ├─> [3] KALIB EL - Kalibracja elewacji\n"
    "│   ├─> [4] ALARMY - Lista aktywnych alarmów\n"
    "│   ├─> [5] INFO - Informacje o systemie\n"
    "│   └─> [6] REMOTE CTRL - Wybór trybu komunikacji zdalnej\n"
    "│       ├─> [0] USBSERIAL - Komendy przez Serial USB (protokół GS-232A)\n"
    "│       └─> [1] TCPIP - Komendy przez WiFi TCP/IP (protokół rotctld/Hamlib)\n\n"
    
    "NAWIGACJA:\n"
    "----------\n"
    "- Krótkie kliknięcie przycisku: wejście do menu / akcja\n"
    "- Długie kliknięcie (>400ms): powrót do poprzedniego menu\n"
    "- Enkoder: nawigacja w menu / zmiana wartości\n\n"
    
    "================================================================================\n\n"
    
    "3. KOMUNIKACJA Z KOMPUTEREM\n"
    "================================================================================\n\n"
    
    "TRYBY KOMUNIKACJI ZDALNEJ (Menu → REMOTE CTRL):\n"
    "------------------------------------------------\n"
    "1. USBSERIAL - Komunikacja przez Serial USB (port COM)\n"
    "   - Protokół: Yaesu GS-232A\n"
    "   - Prędkość: 57600 bps\n"
    "   - Komendy: C, M, B, W, S, A, E, L, R, U, D\n"
    "   - Użycie: Orbitron, Gpredict, SatPC32 → Port COM (bez logów debug)\n\n"
    
    "2. TCPIP - Komunikacja przez WiFi TCP/IP\n"
    "   - Protokół: rotctld/Hamlib\n"
    "   - Port TCP: 4533\n"
    "   - Tryb WiFi: AP (SSID: ROTORSP3KON) lub połączenie z siecią domową\n"
    "   - Komendy: P, p, \\get_pos, \\set_pos, S, \\stop\n"
    "   - Kompatybilny z: SkyROOF, Tucnak (Remote rotator)\n"
    "   - Strona WWW: http://IP_urzadzenia (konfiguracja parametrów i WiFi)\n\n"
    
    "WAŻNE: Sterowanie z komputera działa tylko w trybie AUTO.\n\n"
    
    "================================================================================\n\n"
    
    "4. NOWE FUNKCJE I ZMIANY W WERSJI 3.2:\n"
    "================================================================================\n"
    "- Automatyczne bazowanie (homing) do pozycji 0/0 po uruchomieniu urządzenia\n"
    "- Nowa strona WWW (http://IP_urzadzenia) z panelem sterowania (Manual/Auto)\n"
    "- Sterowanie strzałkami i ustawianie azymutu/elewacji przez przeglądarkę\n"
    "- Rozdzielenie protokołów: GS-232A (Serial USB) / rotctld (TCP/IP)\n"
    "- Mechanizm zabezpieczający przed oscylacjami (3 próby dążenia do celu)\n"
    "- Odwrócenie skali PAN/TILT (Normal/Reverse) - mapowanie współrzędnych\n"
    "- WiFi: Automatyczne połączenie z siecią domową lub tryb Access Point\n"
    "- Menu INFO na OLED wyświetla aktualny adres IP urządzenia\n"
    "- WiFi Sleep wyłączone dla poprawy stabilności połączeń TCP\n\n"
    
    "================================================================================\n\n"
    
    "Wersja oprogramowania: 3.2 (2026-01-19)\n"
    "Kompatybilność: ESP32 WROOM-32, WROVER-E, DevKitC\n"
    "================================================================================\n";
}

void handleSaveWiFi() {
  if (webServer.hasArg("ssid")) {
    String ssid = webServer.arg("ssid");
    String password = webServer.hasArg("pass") ? webServer.arg("pass") : "";
    prefs.putString("wifiSSID", ssid);
    prefs.putString("wifiPassword", password);
    webServer.send(200, "text/html", "<html><body><h1>WiFi Settings Saved!</h1><p>Restart device to apply changes.</p><p><a href='/'>Back</a></p></body></html>");
  } else {
    webServer.send(400, "text/html", "<html><body><h1>Error</h1><p><a href='/'>Back</a></p></body></html>");
  }
}

void setupWebServer() {
  webServer.on("/", handleRoot);
  webServer.on("/save", handleSaveSettings);
  webServer.on("/savewifi", handleSaveWiFi);
  webServer.on("/info", handleInfo);
  webServer.on("/toggleAuto", handleToggleAuto);
  webServer.on("/manual", handleManualCmd);
  webServer.on("/setpos", handleSetPos);
  webServer.on("/api/position", handlePositionAPI);
  webServer.begin();
}
