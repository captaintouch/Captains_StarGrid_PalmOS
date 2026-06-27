#ifndef I_DEVICE_H_
#define I_DEVICE_H_

#include "../game/models.h"

typedef int IBool;

IBool idevice_colorSupported();
long idevice_currentDepth();
unsigned long idevice_maxDepth();
IBool idevice_isRunningMinimalOSVersion(unsigned char minVersion);
IBool idevice_supportsHiDensity();
Coordinate idevice_screenSize(void);
IBool idevice_diaSupported();
unsigned long idevice_setScreenDepth(unsigned long depth);

/* Desired hex-grid tile size in pixels for this platform/screen, enabling the
   grid to scale (zoom) on roomier displays. Return 0 to keep the game's default
   fixed tile size (the shared code clamps the result to its zoom limits). */
int idevice_gridTileSize(Coordinate screenSize);

#endif
