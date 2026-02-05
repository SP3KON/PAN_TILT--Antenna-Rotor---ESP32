#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "config.h"

// Web server handler functions
void handleRoot();
void handleSaveSettings();
void handleToggleAuto();
void handleManualCmd();
void handleSetPos();
void handlePositionAPI();
void handleInfo();
void handleSaveWiFi();
void setupWebServer();

// Helper function
String readInfoFile();

#endif // WEBSERVER_H
