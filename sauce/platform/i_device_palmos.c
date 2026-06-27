#include "i_device.h"

#include <PalmOS.h>

IBool idevice_colorSupported() {
    Boolean supportsColor = false;
    WinScreenMode(winScreenModeGetSupportsColor, NULL, NULL, NULL, &supportsColor);
    return supportsColor;
}

long idevice_currentDepth() {
    UInt32 depth = -1;
    WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
    return depth;
}

unsigned long idevice_maxDepth() {
    UInt32 supportedDepths = 0;
    WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &supportedDepths, NULL);
    if (supportedDepths & 0x80) {
        return 8;
    } else if (supportedDepths & 0x0B) {
        return 4;
    } else {
        return -1;
    }
}

IBool idevice_isRunningMinimalOSVersion(unsigned char minVersion) {
    UInt32 romVersion;
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    return romVersion >= sysMakeROMVersion(minVersion, 0, 0, sysROMStageRelease, 0);
}

IBool idevice_supportsHiDensity() {
    UInt32 attr;
    WinScreenGetAttribute(winScreenDensity, &attr);
    return (attr == kDensityDouble);
}

Coordinate idevice_screenSize() {
    RectangleType screenBounds;
    idevice_isRunningMinimalOSVersion(4) ? WinGetBounds(WinGetDisplayWindow(), &screenBounds) : WinGetDrawWindowBounds(&screenBounds);
    return (Coordinate){screenBounds.extent.x, screenBounds.extent.y};
}

IBool idevice_diaSupported() {
    UInt32 version;
    Err err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
    return (!err && version);
}

int idevice_gridTileSize(Coordinate screenSize) {
    /* Palm OS keeps the historical fixed tile size; larger Palm screens are
       handled by viewport scrolling, not by zooming the grid. */
    (void)screenSize;
    return 0;
}

unsigned long idevice_setScreenDepth(unsigned long depth) {
    UInt32 oldDepth = idevice_currentDepth();
    UInt32 requestedDepth = depth;
    if (requestedDepth < 4 || WinScreenMode(winScreenModeSet, NULL, NULL, &requestedDepth, NULL) != errNone) {
        return 0;
    }
    return oldDepth;
}
