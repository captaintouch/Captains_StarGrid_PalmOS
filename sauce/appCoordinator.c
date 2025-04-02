#include "appCoordinator.h"
#include "game/game.h"
#include <PalmOS.h>

Activity currentActivity = GAME;

// STATIC FUNCTION DEFINITIONS
static Boolean appCoordinator_handle(Activity activity, EventPtr eventptr);
static int appCoordinator_eventDelayTime(Activity activity);
static void appCoordinator_launchGame();
static void appCoordinator_launchStartScreen();

// PUBLIC
void appCoordinator_cleanup() {
    FormType *formP;
    switch (currentActivity) {
    case STARTSCREEN:
        
        break;
    case GAME:
        game_cleanup();
        break;
    }

    formP = FrmGetActiveForm();
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

void appCoordinator_startEventDispatcher(Activity activity) {
    EventType event;
    UInt16 err;

    appCoordinator_launchGame();

    do {
        EvtGetEvent(&event, appCoordinator_eventDelayTime(currentActivity));
        if (!SysHandleEvent(&event)) {
            if (!MenuHandleEvent(NULL, &event, &err)) {
                if (!appCoordinator_handle(currentActivity, &event)) {
                    FrmDispatchEvent(&event);
                }
            }
        }

    } while (event.eType != appStopEvent);
}

// CALLBACKS

static void appCoordinator_launchGame() {
    FormType *formP = FrmGetActiveForm();
    game_setup();
    currentActivity = GAME;
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

// STATIC

static Boolean appCoordinator_handle(Activity activity, EventPtr eventptr) {
    switch (activity) {
    case GAME:
        return game_mainLoop(eventptr, appCoordinator_launchStartScreen);
    case STARTSCREEN:
        break;
    default:
        break;
    }
    return false;
}

static int appCoordinator_eventDelayTime(Activity activity) {
    switch (activity) {
    case GAME:
        return game_eventDelayTime();
    case STARTSCREEN:
        return 0;
    }
}

static void appCoordinator_launchStartScreen() {
    

}