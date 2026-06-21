#include "deviceinfo.h"

#include "platform/i_device.h"
#include "platform/i_system.h"

Boolean deviceinfo_colorSupported() {
    return idevice_colorSupported();
}

Int32 deviceinfo_currentDepth() {
    return idevice_currentDepth();
}

UInt32 deviceinfo_maxDepth() {
    return idevice_maxDepth();
}

Boolean deviceinfo_isRunningMinimalOSVersion(UInt8 minVersion) {
    return idevice_isRunningMinimalOSVersion(minVersion);
}

Boolean deviceinfo_supportsHiDensity() {
    return idevice_supportsHiDensity();
}

Coordinate deviceinfo_screenSize() {
    return idevice_screenSize();
}

Boolean deviceinfo_diaSupported() {
    return idevice_diaSupported();
}

void sleep(UInt32 milliseconds) {
    isys_sleepMs(milliseconds);
}
