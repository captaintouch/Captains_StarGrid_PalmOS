#include "gamesession.h"
#include "../constants.h"

void gameSession_registerPenInput(EventPtr eventptr) {
    // Input we get is for the entire screen, we need to offset it so that it matches our playing field area
    inputPen_updateEventDetails(&gameSession.lastPenInput, eventptr, -GAMEWINDOW_X, -GAMEWINDOW_Y);
}

void gameSession_progressLogic() {
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        gameSession.lastPenInput.wasUpdatedFlag = false;

        
    }
}