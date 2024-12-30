#include "game.h"

#include <PalmOS.h>

#include "../MathLib.h"
#include "../constants.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "spriteLibrary.h"

WinHandle backgroundBuffer = NULL;

int game_eventDelayTime() {
    return 0;
}

void game_setup() {
    spriteLibrary_initialize();
}

static void game_drawSelectedTile() {
    Coordinate selectedTile = hexgrid_tileAtPixel(gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y);
    //drawhelper_applyForeColor(EMERALD);
    //hexgrid_drawTileAtPosition(selectedTile);
    hexgrid_fillTileAtPosition(selectedTile, EMERALD, backgroundBuffer);
    drawhelper_applyForeColor(ALIZARIN);
}

static void game_drawPawns() {
    hexgrid_drawSpriteAtTile(&spriteLibrary.shipSprite, (Coordinate){2, 2});
}

static void game_drawBackdrop() {
    int i;
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&(RectangleType){0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT});
    
    // Draw stars at random locations
    for (i = 0; i < BACKDROP_STARCOUNT; i++) {
        if (i % 4 == 0) {
            drawhelper_applyForeColor(ASBESTOS);
        } else if (i % 6 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }

        WinDrawPixel(SysRandom(0) % GAMEWINDOW_WIDTH, SysRandom(0) % GAMEWINDOW_HEIGHT);
    }
}

static WinHandle game_drawBackground() {
    Err err = errNone;
    if (backgroundBuffer != NULL) {
        WinDeleteWindow(backgroundBuffer, false);
    }
    backgroundBuffer = WinCreateOffscreenWindow(GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);
    WinSetDrawWindow(backgroundBuffer);

    game_drawBackdrop();
    hexgrid_drawEntireGrid();
    game_drawSelectedTile();
    game_drawPawns();
    return backgroundBuffer;
}

static void game_drawLayout() {
    RectangleType lamerect;
    WinHandle mainWindow = WinGetDrawWindow();
    Err err = errNone;
    WinHandle backgroundBuffer;
    WinHandle screenBuffer = WinCreateOffscreenWindow(GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);
    WinSetDrawWindow(screenBuffer);
    backgroundBuffer = game_drawBackground();
    RctSetRectangle(&lamerect, 0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(backgroundBuffer, screenBuffer, &lamerect, 0, 0, winPaint);
    RctSetRectangle(&lamerect, 0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, GAMEWINDOW_X,
                     GAMEWINDOW_Y,
                     winPaint);
    WinSetDrawWindow(mainWindow);
    WinDeleteWindow(screenBuffer, false);
}

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t requestMainMenu) {
    gameSession_registerPenInput(eventptr);
    if (eventptr->eType != nilEvent)
        return false;
    game_drawLayout();
    gameSession_progressLogic();
    return true;
}
