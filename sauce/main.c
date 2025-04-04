#include "appCoordinator.h"
#include <PalmOS.h>
#include "appCoordinator.h"
#include "deviceinfo.h"
#include "game/colors.h"
#include <PalmOS.h>

static UInt32 applyScreenMode() {
    Int32 oldDepth = deviceinfo_currentDepth();
    Int32 depth = deviceinfo_maxDepth();
    if (depth < 4 || WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL) != errNone) {
        ErrFatalDisplay("Unsupported device");
    }
    
    colors_setupReferenceColors(deviceinfo_colorSupported(), depth);
    return oldDepth;
}

static void checkHiResSupport() {
    #ifdef HIRESBUILD
    if (!deviceinfo_isRunningMinimalOSVersion(5) || !deviceinfo_supportsHiDensity()) {
        ErrFatalDisplay("Please install lowres version");
    }
    #endif
}

static UInt32 startApplication() {
    checkHiResSupport();
    return applyScreenMode();
}

static void endApplication(UInt32 oldDepth) {
    appCoordinator_cleanup();
    WinScreenMode(winScreenModeSet, NULL, NULL, &oldDepth, NULL);
}

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
    UInt32 oldDepth;
    if (cmd == sysAppLaunchCmdNormalLaunch) {
        oldDepth = startApplication();
        appCoordinator_startEventDispatcher(GAME);
        endApplication(oldDepth);
    }
    return 0;
}
