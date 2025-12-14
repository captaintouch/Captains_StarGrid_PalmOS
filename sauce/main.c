#include <PalmOS.h>

#include "deviceinfo.h"
#include "game/colors.h"
#include "game/game.h"

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
    if (!deviceinfo_isRunningMinimalOSVersion(4) || !deviceinfo_supportsHiDensity()) {
        ErrFatalDisplay("Please install lowres version");
    }
#endif
}

static UInt32 startApplication() {
    checkHiResSupport();
    return applyScreenMode();
}

static void runGameEventLoop() {
    EventType event;
    UInt16 err;

    FormType *formP = FrmGetActiveForm();
    game_setup();
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }

    do {
        EvtGetEvent(&event, game_eventDelayTime());
        if (!SysHandleEvent(&event)) {
            if (!MenuHandleEvent(NULL, &event, &err)) {
                if (!game_mainLoop(&event, NULL)) {
                    FrmDispatchEvent(&event);
                }
            }
        }
    } while (event.eType != appStopEvent);
}

static void cleanupGame() {
    FormType *formP = FrmGetActiveForm();
    game_cleanup();
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

static void endApplication(UInt32 oldDepth) {
    cleanupGame();
    WinScreenMode(winScreenModeSet, NULL, NULL, &oldDepth, NULL);
}

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
    UInt32 oldDepth;
    if (cmd == sysAppLaunchCmdNormalLaunch) {
        oldDepth = startApplication();
        runGameEventLoop();
        endApplication(oldDepth);
    }
    return 0;
}
