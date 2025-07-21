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

UInt32 deviceinfo_maxDepth() {
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

Boolean deviceinfo_isRunningMinimalOSVersion(UInt8 minVersion) {
    UInt32 romVersion;
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion); 
    return romVersion >= sysMakeROMVersion(minVersion, 0, 0, sysROMStageRelease, 0);
}

Boolean deviceinfo_supportsHiDensity() {
    UInt32 attr;
    WinScreenGetAttribute(winScreenDensity, &attr); 
    return (attr == kDensityDouble);
}

Coordinate deviceinfo_screenSize() {
    RectangleType screenBounds;
    deviceinfo_isRunningMinimalOSVersion(4) ? WinGetBounds(WinGetDisplayWindow(), &screenBounds) : WinGetDrawWindowBounds(&screenBounds);
    return (Coordinate){screenBounds.extent.x, screenBounds.extent.y};
}

Boolean deviceinfo_diaSupported() {
    UInt32 version;
    Err err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
    return (!err && version);
}

void sleep(UInt32 milliseconds) {
    UInt32 startTicks = TimGetTicks();
    UInt32 delayTicks = (milliseconds * SysTicksPerSecond()) / 1000;

    EventType event;
    while ((TimGetTicks() - startTicks) < delayTicks) {
        EvtGetEvent(&event, delayTicks);
        if (event.eType != nilEvent) {
            EvtAddEventToQueue(&event);  // Put it back if needed
        }
    }
}

