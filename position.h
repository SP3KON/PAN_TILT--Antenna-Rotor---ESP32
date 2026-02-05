#ifndef POSITION_H
#define POSITION_H

#include "config.h"

// Position conversion functions
int getGeographicAzimuth();
int getUserElevation();
int geographicToInternal(int geoAZ);
int userToInternalEL(int userEL);
int internalToGeographic(int internalAz);

#endif // POSITION_H
