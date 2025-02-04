#include "game.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "minimap.h"
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
        gameSession.drawingState.shouldRedrawOverlay = true;
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

static void game_drawAttackAnimation() {
    int i;
    if (gameSession.attackAnimation == NULL) {
        return;
    }
    
    drawhelper_applyForeColor(ALIZARIN);
    for (i = 0; i < gameSession.attackAnimation->lineCount; i++) {
        if (i % 2 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(SUNFLOWER);
        }
        drawhelper_drawLine(&gameSession.attackAnimation->lines[i]);
    }
}

static void game_drawHighlightTiles() {  // Tiles that need to be highlighted (for example to indicate where a pawn can move)
    int i;
    if (gameSession.secondaryHighlightTiles != NULL && gameSession.secondaryHighlightTileCount > 0) {
        drawhelper_applyForeColor(gameSession_hightlightTilesColor());
        for (i = 0; i < gameSession.secondaryHighlightTileCount; i++) {
            hexgrid_drawTileAtPosition(gameSession.secondaryHighlightTiles[i]);
        }
    }
    if (gameSession.highlightTiles != NULL && gameSession.highlightTileCount > 0) {
        drawhelper_applyForeColor(gameSession_hightlightTilesColor());
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            hexgrid_fillTileAtPosition(gameSession.highlightTiles[i]);
        }
        drawhelper_applyForeColor(CLOUDS);
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            hexgrid_drawTileAtPosition(gameSession.highlightTiles[i]);
        }
    }
}

static void game_drawPawns() {
    int i;
    if (gameSession.activePawn != NULL) {
        drawhelper_applyForeColor(EMERALD);
        hexgrid_drawTileAtPosition(gameSession.activePawn->position);
    }

    // DRAW SHIPS
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        Coordinate pawnPosition;
        RectangleType flagRect;
        ImageSprite *shipSprite;
        if (pawn->type != PAWNTYPE_SHIP) {
            continue;
        }
        pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        if (pawn->cloaked) {
            shipSprite = &spriteLibrary.shipCloakedSprite[pawn->orientation];
        } else {
            shipSprite = &spriteLibrary.shipSprite[pawn->orientation];
        }

        if (gameSession.movement->pawn == pawn) {
            drawhelper_drawSprite(shipSprite, gameSession.movement->pawnPosition);
        } else {
            hexgrid_drawSpriteAtTile(shipSprite, pawn->position);
            // Draw faction flag
            if (gameSession.colorSupport) {
                RctSetRectangle(&flagRect, pawnPosition.x + 5, pawnPosition.y - 10, 5, 5);
                drawhelper_applyForeColor(gameSession_factionColor(pawn->faction));
                drawhelper_fillRectangle(&flagRect, 0);
            } else {
                drawhelper_applyForeColor(CLOUDS);
                drawhelper_drawTextWithValue("", pawn->faction + 1, (Coordinate){pawnPosition.x + 10, pawnPosition.y - 10});
            }
        }
    }

    // DRAW FLAGS
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        Coordinate pawnPosition;
        RectangleType flagRect;
        if (pawn->type != PAWNTYPE_FLAG) {
            continue;
        }
        pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        RctSetRectangle(&flagRect, pawnPosition.x - 6, pawnPosition.y - 6, 12, 12);
        drawhelper_applyForeColor(gameSession_factionColor(pawn->faction));
        drawhelper_fillRectangle(&flagRect, 0);
        hexgrid_drawSpriteAtTile(&spriteLibrary.flagSprite, pawn->position);
    }
}

static void game_drawDebugTrajectoryMovement() {
#ifdef DEBUG
    int i;
    if (gameSession.movement == NULL) {
        return;
    }

    for (i = 0; i < gameSession.movement->trajectory.tileCount; i++) {
        Coordinate currentPosition = gameSession.movement->trajectory.tileCoordinates[i];
        char finalText[20];
        char valueText[20];
        if (i % 2 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(EMERALD);
        }

        StrIToA(finalText, currentPosition.x);
        StrCat(finalText, ",");
        StrIToA(valueText, currentPosition.y);
        StrCat(finalText, valueText);
        drawhelper_drawText(finalText, hexgrid_tileCenterPosition(currentPosition));
        if (i > 0) {
            drawhelper_drawLineBetweenCoordinates(hexgrid_tileCenterPosition(gameSession.movement->trajectory.tileCoordinates[i]), hexgrid_tileCenterPosition(gameSession.movement->trajectory.tileCoordinates[i - 1]));
        }
    }
#endif
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
    drawhelper_fillRectangle(&rect, 0);

    // Draw stars at random locations
    for (i = 0; i < BACKDROP_STARCOUNT; i++) {
        if (i % 4 == 0) {
            drawhelper_applyForeColor(ASBESTOS);
        } else if (i % 6 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }

        drawhelper_drawPoint((Coordinate){SysRandom(0) % gridSize.x, SysRandom(0) % gridSize.y});
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
    if (!gameSession.drawingState.shouldRedrawOverlay && overlayBuffer != NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = false;
    gridSize = hexgrid_size();
    if (overlayBuffer == NULL) {
        overlayBuffer = WinCreateOffscreenWindow(gridSize.x, gridSize.y, screenFormat, &err);
    }

    WinSetDrawWindow(overlayBuffer);
    RctSetRectangle(&lamerect, 0, 0, gridSize.x, gridSize.y);
    WinCopyRectangle(backgroundBuffer, overlayBuffer, &lamerect, 0, 0, winPaint);

    game_drawHighlightTiles();
    game_drawPawns();
    game_drawAttackAnimation();
    game_drawDebugTrajectoryMovement();
}

static void game_updateMiniMapDrawPosition() {
    Coordinate screenSize = deviceinfo_screenSize();
    int width = (float)screenSize.x * 0.5;
    int centerOffsetX = (screenSize.x - width) / 2;
    gameSession.drawingState.miniMapDrawPosition = (Coordinate){centerOffsetX, screenSize.y - MINIMAP_HEIGHT + 2};
    gameSession.drawingState.miniMapSize = (Coordinate){width, MINIMAP_HEIGHT - 2};
}

static void game_drawMiniMap() {
    minimap_draw(gameSession.pawns,
                 gameSession.pawnCount,
                 gameSession.drawingState.miniMapDrawPosition,
                 gameSession.drawingState.miniMapSize,
                 gameSession.movement,
                 gameSession.activePawn,
                 gameSession.viewportOffset);
}

static void game_drawBottomBackground() {
    Coordinate screenSize = deviceinfo_screenSize();
    int width = (float)screenSize.x * 0.5;
    int centerOffsetX = (screenSize.x - width) / 2;
    RectangleType rect;
    RctSetRectangle(&rect, 0, screenSize.y - BOTTOMMENU_HEIGHT, screenSize.x, BOTTOMMENU_HEIGHT);
    drawhelper_applyForeColor(BELIZEHOLE);
    drawhelper_fillRectangle(&rect, 0);
    drawhelper_applyForeColor(CLOUDS);
    RctSetRectangle(&rect, centerOffsetX - 2, screenSize.y - MINIMAP_HEIGHT, width + 4, MINIMAP_HEIGHT + 10);
    drawhelper_fillRectangle(&rect, 4);
}

static void game_drawUserInterfaceElements() {
    game_updateMiniMapDrawPosition();
    game_drawBottomBackground();
    game_drawMiniMap();
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
    RctSetRectangle(&lamerect, gameSession.viewportOffset.x, gameSession.viewportOffset.y, screenSize.x, screenSize.y - BOTTOMMENU_HEIGHT);
    WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

    game_drawUserInterfaceElements();

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
