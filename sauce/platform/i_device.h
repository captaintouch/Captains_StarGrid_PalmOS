#ifndef I_DEVICE_H_
#define I_DEVICE_H_

#include "../game/models.h"

typedef int IBool;

IBool idevice_colorSupported();
long idevice_currentDepth();
unsigned long idevice_maxDepth();
IBool idevice_isRunningMinimalOSVersion(unsigned char minVersion);
IBool idevice_supportsHiDensity();
Coordinate idevice_screenSize();
IBool idevice_diaSupported();
unsigned long idevice_setScreenDepth(unsigned long depth);

#endif
