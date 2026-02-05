#include "position.h"

// Konwersja wewnętrznej pozycji na azymut geograficzny (0-359)
int getGeographicAzimuth() {
  // Jeśli reverse, odwróć skalę wewnętrzną (PAN_MAX - current)
  int physAZ = reversePAN ? (PAN_MAX - currentAZ) : currentAZ;
  int geoAZ = (physAZ + direction) % 360;
  if (geoAZ < 0) geoAZ += 360;
  return geoAZ;
}

// Pobierz aktualną elewację dla użytkownika (z uwzględnieniem reverse)
int getUserElevation() {
  return reverseTILT ? (TILT_MAX - currentEL) : currentEL;
}

// Konwersja azymutu geograficznego na wewnętrzną pozycję (0-PAN_MAX)
int geographicToInternal(int geoAZ) {
  // Oblicz dystans geograficzny od punktu DIR
  int dist = (geoAZ - direction + 360) % 360;
  
  // Jeśli rotor ma zakładkę (PAN_MAX > 360), sprawdź czy cel jest w zakładce
  if (PAN_MAX > 360) {
    if (dist + 360 <= PAN_MAX) {
      // Wybieramy opcję bliższą obecnej pozycji (uwzględniając ewentualne reverse później)
      int opt1 = reversePAN ? (PAN_MAX - dist) : dist;
      int opt2 = reversePAN ? (PAN_MAX - (dist + 360)) : (dist + 360);
      if (abs(opt2 - currentAZ) < abs(opt1 - currentAZ)) {
        dist += 360;
      }
    }
  }
  
  return reversePAN ? (PAN_MAX - dist) : dist;
}

// Konwersja elewacji użytkownika na wewnętrzną pozycję (0-TILT_MAX)
int userToInternalEL(int userEL) {
  int val = reverseTILT ? (TILT_MAX - userEL) : userEL;
  return constrain(val, 0, TILT_MAX);
}

// Konwersja wewnętrznej pozycji (np. targetAZ) na azymut geograficzny (0-359)
int internalToGeographic(int internalAz) {
  int physAZ = reversePAN ? (PAN_MAX - internalAz) : internalAz;
  int geoAZ = (physAZ + direction) % 360;
  if (geoAZ < 0) geoAZ += 360;
  return geoAZ;
}
