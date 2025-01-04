#include "game.h"

#include <PalmOS.h>

#include "../constants.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "spriteLibrary.h"

WinHandle backgroundBuffer = NULL;
WinHandle overlayBuffer = NULL;
Boolean shouldRedrawBackground = true;

int game_eventDelayTime() {
    return 0;
}

void game_setup() {
    spriteLibrary_initialize();
    gameSession_initialize();
    hexgrid_initialize();
}

static void game_drawSpecialTiles(WinHandle buffer) {  // Tiles that need to be highlighted (for example to indicate where a pawn can move)
    int i;
    for (i = 0; i < gameSession.specialTileCount; i++) {
        hexgrid_fillTileAtPosition(gameSession.specialTiles[i], EMERALD, buffer);
    }
    drawhelper_applyForeColor(CLOUDS);
    for (i = 0; i < gameSession.specialTileCount; i++) {
        hexgrid_drawTileAtPosition(gameSession.specialTiles[i]);
    }
    drawhelper_applyForeColor(ALIZARIN);
}

static void game_drawPawns() {
    int i;
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        hexgrid_drawSpriteAtTile(&spriteLibrary.shipSprite, pawn->position);
    }

    hexgrid_drawSpriteAtTile(&spriteLibrary.shipSprite, (Coordinate){0, 0});
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
    if (!shouldRedrawBackground && backgroundBuffer != NULL) {
        return backgroundBuffer;
    }
    shouldRedrawBackground = false;
    if (backgroundBuffer != NULL) {
        WinDeleteWindow(backgroundBuffer, false);
    }
    backgroundBuffer = WinCreateOffscreenWindow(GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);
    WinSetDrawWindow(backgroundBuffer);

    game_drawBackdrop();
    hexgrid_drawEntireGrid();
    return backgroundBuffer;
}

static WinHandle game_drawOverlay() { // ships, special tiles, etc.
RectangleType lamerect;
    Err err = errNone;
    if (!gameSession.shouldRedrawOverlay && overlayBuffer != NULL) {
        return overlayBuffer;
    }
    gameSession.shouldRedrawOverlay = false;
    if (overlayBuffer != NULL) {
        WinDeleteWindow(overlayBuffer, false);
    }
    overlayBuffer = WinCreateOffscreenWindow(GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);
    WinSetDrawWindow(overlayBuffer);
    RctSetRectangle(&lamerect, 0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(backgroundBuffer, overlayBuffer, &lamerect, 0, 0, winPaint);

    game_drawSpecialTiles(overlayBuffer);
    game_drawPawns();
    return overlayBuffer;
}

static void game_drawLayout() {
    RectangleType lamerect;
    WinHandle mainWindow = WinGetDrawWindow();
    Err err = errNone;
    WinHandle screenBuffer = WinCreateOffscreenWindow(GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);
    WinSetDrawWindow(screenBuffer);
    game_drawBackground();
    
    game_drawOverlay();
    RctSetRectangle(&lamerect, 0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

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
