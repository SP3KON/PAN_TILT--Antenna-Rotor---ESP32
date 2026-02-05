#ifndef ROTCTLD_H
#define ROTCTLD_H

#include "config.h"

// Protocol handlers
void handleGs232Command(char* cmd, int len, Stream* output, bool logSerial);
void handleRotctldCommand(char* cmd, int len, Stream* output, bool logSerial);

#endif // ROTCTLD_H
