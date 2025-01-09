#include "game.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "spriteLibrary.h"

WinHandle backgroundBuffer = NULL;
WinHandle overlayBuffer = NULL;
WinHandle screenBuffer = NULL;
Boolean shouldRedrawBackground = true;
Coordinate lastScreenSize;

static void game_resetForm() {
    FormType *frmP = FrmGetActiveForm();
    Coordinate screenSize = deviceinfo_screenSize();
    FormType *updatedForm = FrmNewForm(GAME_FORM, NULL, 0, 0, screenSize.x, screenSize.y, true, 0, 0, 0);
    lastScreenSize = screenSize;
    FrmSetActiveForm(updatedForm);
    if (frmP != NULL) {
        FrmDeleteForm(frmP);
    }
    if (gameSession.diaSupport) {
        FrmSetDIAPolicyAttr(updatedForm, frmDIAPolicyCustom);
        if (PINGetInputAreaState() != pinInputAreaClosed) {
            PINSetInputAreaState(pinInputAreaClosed);
        }
        if (PINGetInputTriggerState() != pinInputTriggerDisabled) {
            PINSetInputTriggerState(pinInputTriggerDisabled);
        }
        if (backgroundBuffer != NULL) {
            WinDeleteWindow(backgroundBuffer, false);
            backgroundBuffer = NULL;
        }
        if (overlayBuffer != NULL) {
            WinDeleteWindow(overlayBuffer, false);
            overlayBuffer = NULL;
        }
        if (screenBuffer != NULL) {
            WinDeleteWindow(screenBuffer, false);
            screenBuffer = NULL;
        }
        shouldRedrawBackground = true;
        gameSession.shouldRedrawOverlay = true;
    }
}

int game_eventDelayTime() {
    return 0;
}

void game_setup() {
    spriteLibrary_initialize();
    gameSession_initialize();
    hexgrid_initialize();
    game_resetForm();
}

static void game_drawSpecialTiles() {  // Tiles that need to be highlighted (for example to indicate where a pawn can move)
    int i;
    if (gameSession.specialTiles == NULL || gameSession.specialTileCount == 0) {
        return;
    }
    drawhelper_applyForeColor(gameSession_specialTilesColor());
    for (i = 0; i < gameSession.specialTileCount; i++) {
        hexgrid_fillTileAtPosition(gameSession.specialTiles[i]);
    }
    drawhelper_applyForeColor(CLOUDS);
    for (i = 0; i < gameSession.specialTileCount; i++) {
        hexgrid_drawTileAtPosition(gameSession.specialTiles[i]);
    }
}

static void game_drawPawns() {
    int i;
    if (gameSession.activePawn != NULL && gameSession.specialTileCount == 0) {
        drawhelper_applyForeColor(EMERALD);
        hexgrid_drawTileAtPosition(gameSession.activePawn->position);
    }

    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        hexgrid_drawSpriteAtTile(&spriteLibrary.shipSprite, pawn->position);
    }
}

static void game_drawBottomMenu() {
    if (gameSession.displayButtonCount <= 0) {
        return;
    }
    bottomMenu_display(gameSession.displayButtons, gameSession.displayButtonCount);
}

static void game_drawBackdrop() {
    int i;
    Coordinate gridSize = hexgrid_size();
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, gridSize.x, gridSize.y);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect);

    // Draw stars at random locations
    for (i = 0; i < BACKDROP_STARCOUNT; i++) {
        if (i % 4 == 0) {
            drawhelper_applyForeColor(ASBESTOS);
        } else if (i % 6 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }

        WinDrawPixel(SysRandom(0) % gridSize.x, SysRandom(0) % gridSize.y);
    }
}

static void game_drawBackground() {
    Err err = errNone;
    Coordinate gridSize;
    if (!shouldRedrawBackground && backgroundBuffer != NULL) {
        return;
    }
    shouldRedrawBackground = false;
    gridSize = hexgrid_size();
    if (backgroundBuffer == NULL) {
        backgroundBuffer = WinCreateOffscreenWindow(gridSize.x, gridSize.y, screenFormat, &err);
    }

    WinSetDrawWindow(backgroundBuffer);
    game_drawBackdrop();
    hexgrid_drawEntireGrid();
}

static void game_drawOverlay() {  // ships, special tiles, etc.
    RectangleType lamerect;
    Err err = errNone;
    Coordinate gridSize;
    if (!gameSession.shouldRedrawOverlay && overlayBuffer != NULL) {
        return;
    }
    gameSession.shouldRedrawOverlay = false;
    gridSize = hexgrid_size();
    if (overlayBuffer == NULL) {
        overlayBuffer = WinCreateOffscreenWindow(gridSize.x, gridSize.y, screenFormat, &err);
    }

    WinSetDrawWindow(overlayBuffer);
    RctSetRectangle(&lamerect, 0, 0, gridSize.x, gridSize.y);
    WinCopyRectangle(backgroundBuffer, overlayBuffer, &lamerect, 0, 0, winPaint);

    game_drawSpecialTiles();
    game_drawPawns();
    game_drawBottomMenu();
}

static void game_drawLayout() {
    RectangleType lamerect;
    WinHandle mainWindow = WinGetDisplayWindow();
    Err err = errNone;
    Coordinate screenSize = deviceinfo_screenSize();
    if (screenBuffer == NULL) {
        screenBuffer = WinCreateOffscreenWindow(screenSize.x, screenSize.y, screenFormat, &err);
    }
    game_drawBackground();
    game_drawOverlay();

    WinSetDrawWindow(screenBuffer);
    RctSetRectangle(&lamerect, gameSession.viewportOffset.x, gameSession.viewportOffset.y, screenSize.x, screenSize.y);
    WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

    RctSetRectangle(&lamerect, 0, 0, screenSize.x, screenSize.y);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, GAMEWINDOW_X,
                     GAMEWINDOW_Y,
                     winPaint);
    WinSetDrawWindow(mainWindow);
}

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t requestMainMenu) {
    gameSession_registerPenInput(eventptr);
    if (eventptr->eType == winDisplayChangedEvent) {
        game_resetForm();
        return true;
    }
    if (eventptr->eType != nilEvent)
        return false;

    game_drawLayout();
    gameSession_progressLogic();
    return true;
}
