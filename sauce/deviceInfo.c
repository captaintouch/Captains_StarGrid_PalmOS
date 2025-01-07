#include "deviceinfo.h"
#include <PalmOS.h>

Boolean deviceinfo_colorSupported() {
    Boolean supportsColor = false;
    WinScreenMode(winScreenModeGetSupportsColor, NULL, NULL, NULL, &supportsColor);
    return supportsColor;
}

Int32 deviceinfo_currentDepth() {
    UInt32 depth = -1;
    WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
    return depth;
}

Int32 deviceinfo_maxDepth() {
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

Coordinate deviceinfo_screenSize() {
    RectangleType screenBounds;
    WinGetBounds(WinGetDisplayWindow(), &screenBounds);
    return (Coordinate){screenBounds.extent.x, screenBounds.extent.y};
}

Boolean deviceinfo_diaSupported() {
    UInt32 version;
    Err err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
    return (!err && version);
}