#include <PalmOS.h>

#include "platform/i_device.h"
#include "platform/i_system.h"
#include "platform/i_ui.h"
#include "game/colors.h"
#include "game/game.h"

static unsigned long applyScreenMode() {
    unsigned long depth = idevice_maxDepth();
    unsigned long oldDepth = idevice_setScreenDepth(depth);
    if (depth < 4 || oldDepth == 0) {
        isys_fatalError("Unsupported device");
    }

    colors_setupReferenceColors(idevice_colorSupported(), depth);
    return oldDepth;
}

static void checkHiResSupport() {
#ifdef HIRESBUILD
    if (!idevice_isRunningMinimalOSVersion(4) || !idevice_supportsHiDensity()) {
        isys_fatalError("Please install lowres version");
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

    IForm *formP = iui_activeForm();
    game_setup();
    if (formP != NULL) {
        iui_deleteForm(formP);
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
    IForm *formP = iui_activeForm();
    game_cleanup();
    if (formP != NULL) {
        iui_deleteForm(formP);
    }
}

static void endApplication(UInt32 oldDepth) {
    cleanupGame();
    idevice_setScreenDepth(oldDepth);
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
