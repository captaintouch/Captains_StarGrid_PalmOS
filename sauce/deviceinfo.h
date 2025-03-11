#ifndef DEVICEINFO_H_
#define DEVICEINFO_H_
#include <PalmOS.h>
#include "game/models.h"

Boolean deviceinfo_colorSupported();
Int32 deviceinfo_maxDepth();
Int32 deviceinfo_currentDepth();
Coordinate deviceinfo_screenSize();
Boolean deviceinfo_diaSupported();
void sleep(UInt32 milliseconds);

#endif