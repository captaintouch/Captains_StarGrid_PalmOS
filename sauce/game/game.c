#include "game.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../deviceinfo.h"
#include "../graphicResources.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "minimap.h"
#include "pawn.h"
#include "spriteLibrary.h"
#include "viewport.h"

WinHandle backgroundBuffer = NULL;
WinHandle overlayBuffer = NULL;
WinHandle screenBuffer = NULL;
Boolean shouldRedrawBackground = true;
Coordinate lastScreenSize;

static void game_windowCleanup() {
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
}

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
        game_windowCleanup();
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

void game_cleanup() {
    game_windowCleanup();
    gameSession_cleanup();
}

static void game_drawWarpAnimation() {
    int i;
    Coordinate targetPosition;
    if (!gameSession.warpAnimation.isWarping) {
        return;
    }
    targetPosition = hexgrid_tileCenterPosition(gameSession.warpAnimation.currentPosition);
    for (i = 0; i < WARPCIRCLECOUNT; i++) {
        if (i % 2 == 0) {
            drawhelper_applyForeColor(BELIZEHOLE);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }
        drawhelper_drawCircle(viewport_convertedCoordinate(targetPosition), gameSession.warpAnimation.circleDiameter[i]);
    }
}

static void game_drawAttackAnimation() {
    int i;
    if (gameSession.attackAnimation == NULL) {
        return;
    }
    if (!isInvalidCoordinate(gameSession.attackAnimation->explosionPosition)) {
        drawhelper_drawAnimatedSprite(spriteLibrary.explosionAnimation, GFX_FRAMECOUNT_EXPLOSION, viewport_convertedCoordinate(gameSession.attackAnimation->explosionPosition), gameSession.attackAnimation->explosionTimestamp, gameSession.attackAnimation->explosionDurationSeconds);
        return;
    }
    if (isInvalidCoordinate(gameSession.attackAnimation->torpedoPosition)) {  // Phaser animation
        drawhelper_applyForeColor(ALIZARIN);
        for (i = 0; i < gameSession.attackAnimation->lineCount; i++) {
            Line targetLine = viewport_convertedLine(gameSession.attackAnimation->lines[i]);
            if (i % 2 == 0) {
                drawhelper_applyForeColor(ALIZARIN);
            } else {
                drawhelper_applyForeColor(SUNFLOWER);
            }
            drawhelper_drawLine(&targetLine);
        }
    } else {  // Torpedo animation
        drawhelper_drawAnimatedSprite(spriteLibrary.torpedoAnimation, GFX_FRAMECOUNT_TORP, viewport_convertedCoordinate(gameSession.attackAnimation->torpedoPosition), gameSession.attackAnimation->launchTimestamp, gameSession.attackAnimation->durationSeconds);
    }
}

static void game_drawHighlightTiles() {  // Tiles that need to be highlighted (for example to indicate where a pawn can move)
    int i;
    if (gameSession.secondaryHighlightTiles != NULL && gameSession.secondaryHighlightTileCount > 0) {
        drawhelper_applyForeColor(gameSession_hightlightTilesColor());
        for (i = 0; i < gameSession.secondaryHighlightTileCount; i++) {
            hexgrid_drawTileAtPosition(gameSession.secondaryHighlightTiles[i], true);
        }
    }
    if (gameSession.highlightTiles != NULL && gameSession.highlightTileCount > 0) {
        drawhelper_applyForeColor(gameSession_hightlightTilesColor());
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            hexgrid_fillTileAtPosition(gameSession.highlightTiles[i], true);
        }
        drawhelper_applyForeColor(CLOUDS);
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            hexgrid_drawTileAtPosition(gameSession.highlightTiles[i], true);
        }
    }
}

static void game_drawFlag(Coordinate position, AppColor color) {
    RectangleType rectFlag, rectPole;
    RctSetRectangle(&rectFlag, position.x - 10, position.y - 2, 7, 5);
    RctSetRectangle(&rectPole, position.x - 10, position.y + 2, 3, 6);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rectFlag, 0);
    drawhelper_fillRectangle(&rectPole, 0);

    RctSetRectangle(&rectFlag, position.x - 9, position.y - 1, 5, 3);
    RctSetRectangle(&rectPole, position.x - 9, position.y + 1, 1, 5);
    drawhelper_applyForeColor(color);
    drawhelper_fillRectangle(&rectFlag, 0);
    drawhelper_fillRectangle(&rectPole, 0);
}

static ImageSprite *game_spriteForPawn(Pawn *pawn) {
    if (pawn->type == PAWNTYPE_BASE) {
        return &spriteLibrary.baseSprite;
    }
    return &spriteLibrary_factionShipSprite(pawn->faction)[pawn->orientation];
}

static void game_drawHealthBar(Pawn *pawn, int maxWidth, int height, Coordinate position) {
    int maxHealth, healthWidth;
    RectangleType rect;
    if (pawn->type == PAWNTYPE_SHIP) {
        maxHealth = GAMEMECHANICS_MAXSHIPHEALTH;
    } else {
        maxHealth = GAMEMECHANICS_MAXBASEHEALTH;
    }

    // draw border
    drawhelper_applyForeColor(CLOUDS);
    RctSetRectangle(&rect, position.x - 1, position.y - 1, maxWidth + 2, height + 2);
    drawhelper_fillRectangle(&rect, 0);

    healthWidth = (maxWidth * pawn->inventory.health) / maxHealth;
    drawhelper_applyForeColor(ALIZARIN);
    RctSetRectangle(&rect, position.x, position.y, healthWidth, height);
    drawhelper_fillRectangle(&rect, 0);
}

static void game_drawPawns() {
    int i;
    if (gameSession.activePawn != NULL) {
        drawhelper_applyForeColor(EMERALD);
        hexgrid_drawTileAtPosition(gameSession.activePawn->position, true);
    }

    // DRAW BASES
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        Coordinate pawnPosition, pawnPositionConverted;
        RectangleType rect;
        if (pawn->type != PAWNTYPE_BASE || isInvalidCoordinate(gameSession.pawns[i].position)) {
            continue;
        }
        pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        pawnPositionConverted = viewport_convertedCoordinate(pawnPosition);
        RctSetRectangle(&rect, pawnPositionConverted.x - 6, pawnPositionConverted.y - 6, 12, 12);
        drawhelper_applyForeColor(pawn_factionColor(pawn->faction));
        drawhelper_fillRectangle(&rect, 0);
        hexgrid_drawSpriteAtTile(&spriteLibrary.baseSprite, pawn->position);
    }

    // DRAW SHIPS
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        Coordinate pawnPosition;
        ImageSprite *shipSprite;
        if (pawn->type != PAWNTYPE_SHIP || isInvalidCoordinate(gameSession.pawns[i].position)) {
            continue;
        }
        shipSprite = game_spriteForPawn(pawn);
        if (gameSession.warpAnimation.isWarping && gameSession.warpAnimation.pawn == pawn) {
            if (!gameSession.warpAnimation.shipVisible) {
                continue;
            }
            pawnPosition = gameSession.warpAnimation.currentPosition;
        } else {
            pawnPosition = pawn->position;
        }
        if (isInvalidCoordinate(pawnPosition)) {
            continue;
        }
        if (gameSession.movement != NULL && gameSession.movement->pawn == pawn) {
            drawhelper_drawSprite(shipSprite, viewport_convertedCoordinate(gameSession.movement->pawnPosition));
        } else {
            hexgrid_drawSpriteAtTile(shipSprite, pawnPosition);
        }
    }

    // DRAW ACCESSORIES (FLAGS, FACTION INDICATORS)
    for (i = 0; i < gameSession.pawnCount; i++) {
        Pawn *pawn = &gameSession.pawns[i];
        Coordinate pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        if (isInvalidCoordinate(pawn->position)) {
            continue;
        }
        if (pawn->inventory.carryingFlag) {
            game_drawFlag(viewport_convertedCoordinate(pawnPosition), pawn_factionColor(pawn->inventory.flagOfFaction));
        }

        // Draw faction indicator
        if (pawn->type == PAWNTYPE_SHIP) {
            if (gameSession.colorSupport) {
                /*
                RectangleType flagRect;
                Coordinate target = viewport_convertedCoordinate((Coordinate){pawnPosition.x + 5, pawnPosition.y - 10});
                RctSetRectangle(&flagRect, target.x, target.y, 5, 5);
                drawhelper_applyForeColor(pawn_factionColor(pawn->faction));
                drawhelper_fillRectangle(&flagRect, 0);
                */
            } else {
                drawhelper_applyForeColor(CLOUDS);
                drawhelper_drawTextWithValue("", pawn->faction + 1, (Coordinate){pawnPosition.x, pawnPosition.y - 10});
            }
        }

        if (gameSession_shouldShowHealthBar() && gameSession.factionTurn != gameSession.pawns[i].faction) {
            int maxHealthWidth = HEXTILE_PAWNSIZE;
            game_drawHealthBar(&gameSession.pawns[i], maxHealthWidth, 2, viewport_convertedCoordinate((Coordinate){pawnPosition.x - maxHealthWidth / 2, pawnPosition.y + HEXTILE_PAWNSIZE / 2}));
        }
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
        drawhelper_drawText(finalText, viewport_convertedCoordinate(hexgrid_tileCenterPosition(currentPosition)));
        if (i > 0) {
            drawhelper_drawLineBetweenCoordinates(viewport_convertedCoordinate(hexgrid_tileCenterPosition(gameSession.movement->trajectory.tileCoordinates[i])), viewport_convertedCoordinate(hexgrid_tileCenterPosition(gameSession.movement->trajectory.tileCoordinates[i - 1])));
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
    Coordinate gridSize = hexgrid_size();
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, gridSize.x, gridSize.y);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 0);
}

static void game_drawStars() {
    int i;
    Coordinate gridSize = hexgrid_size();

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
        if (err != errNone) {
            backgroundBuffer = NULL;
            return;
        }
    }

    WinSetDrawWindow(backgroundBuffer);
    game_drawBackdrop();
    hexgrid_drawEntireGrid(false);
    game_drawStars();
}

static void game_drawLowMemBackground(Coordinate screenSize) {
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, screenSize.x, screenSize.y);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 0);
    if (!gameSession.drawingState.awaitingEndMiniMapScrolling && !gameSession_animating() && gameSession.state != GAMESTATE_CHOOSEPAWNACTION && gameSession.factions[gameSession.factionTurn].human) {
        hexgrid_drawEntireGrid(true);
    }
}

static void game_drawDynamicViews() {  // ships, special tiles, etc.
    // everything drawn in this function must have it's coordinates offset to the current viewport
    RectangleType lamerect;
    Err err = errNone;
    Coordinate gridSize;
    Coordinate overlaySize = deviceinfo_screenSize();
    overlaySize.y -= BOTTOMMENU_HEIGHT;
    if (!gameSession.drawingState.shouldRedrawOverlay && overlayBuffer != NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = false;
    gridSize = hexgrid_size();
    if (overlayBuffer == NULL) {
        overlayBuffer = WinCreateOffscreenWindow(overlaySize.x, overlaySize.y, screenFormat, &err);
    }

    WinSetDrawWindow(overlayBuffer);
    RctSetRectangle(&lamerect, gameSession.viewportOffset.x, gameSession.viewportOffset.y, overlaySize.x, overlaySize.y);
    if (backgroundBuffer != NULL) {
        WinCopyRectangle(backgroundBuffer, overlayBuffer, &lamerect, 0, 0, winPaint);
    } else {
        // we don't have enough memory for a background buffer, so draw into the overlayBuffer
        game_drawLowMemBackground(overlaySize);
    }

    game_drawHighlightTiles();
    game_drawWarpAnimation();
    game_drawPawns();
    game_drawAttackAnimation();
    game_drawDebugTrajectoryMovement();
}

static void game_updateMiniMapDrawPosition() {
    Coordinate screenSize = deviceinfo_screenSize();
    int width = (float)screenSize.x * 0.4;
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
    int width = (float)screenSize.x * 0.4;
    int centerOffsetX = (screenSize.x - width) / 2;
    RectangleType rect;
    RctSetRectangle(&rect, 0, screenSize.y - BOTTOMMENU_HEIGHT, screenSize.x, BOTTOMMENU_HEIGHT);
    drawhelper_applyForeColor(pawn_factionColor(gameSession.factionTurn));
    drawhelper_fillRectangle(&rect, 0);
    drawhelper_applyForeColor(pawn_factionColor(gameSession.factionTurn));
    RctSetRectangle(&rect, centerOffsetX - 2, screenSize.y - MINIMAP_HEIGHT, width + 4, MINIMAP_HEIGHT + 10);
    drawhelper_fillRectangle(&rect, 4);
}

static void game_drawBottomActivePawn() {
    Coordinate screenSize = deviceinfo_screenSize();
    int offsetX = gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x;
    int offsetY = screenSize.y - BOTTOMMENU_HEIGHT;
    Coordinate targetCenterPosition = (Coordinate){(offsetX + (screenSize.x - offsetX) / 2) - (HEXTILE_PAWNSIZE / 2), offsetY + (BOTTOMMENU_HEIGHT / 2) - (HEXTILE_PAWNSIZE / 2)};
    RectangleType rect;
    Coordinate pawnCenterPosition;
    if (gameSession.activePawn == NULL) {
        return;
    }
    if (gameSession.movement == NULL) {
        pawnCenterPosition = hexgrid_tileCenterPosition(gameSession.activePawn->position);
    } else {
        pawnCenterPosition = gameSession.movement->pawnPosition;
    }

    RctSetRectangle(&rect, targetCenterPosition.x - 2, targetCenterPosition.y - 2, HEXTILE_PAWNSIZE + 4, HEXTILE_PAWNSIZE + 4);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 4);

    pawnCenterPosition = viewport_convertedCoordinate(pawnCenterPosition);
    RctSetRectangle(&rect, pawnCenterPosition.x - HEXTILE_PAWNSIZE / 2, pawnCenterPosition.y - HEXTILE_PAWNSIZE / 2, HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE);
    WinCopyRectangle(overlayBuffer, screenBuffer, &rect, targetCenterPosition.x, targetCenterPosition.y, winPaint);

    if (!gameSession.factions[gameSession.factionTurn].human) {  // draw cpu action text
        int textWidth = FntCharsWidth(gameSession.cpuActionText, StrLen(gameSession.cpuActionText));
        drawhelper_applyTextColor(CLOUDS);
        drawhelper_applyBackgroundColor(DRACULAORCHID);
        drawhelper_drawText(gameSession.cpuActionText, (Coordinate){screenSize.x / 2 - textWidth / 2, screenSize.y - 12});
    }
}

static void game_drawBottomActivePawnStats() {
    Coordinate screenSize = deviceinfo_screenSize();
    int i;
    if (gameSession.activePawn == NULL) {
        return;
    }

    drawhelper_drawSprite(&spriteLibrary.healthSprite, (Coordinate){8, screenSize.y - BOTTOMMENU_HEIGHT + 8});
    game_drawHealthBar(gameSession.activePawn, 28, 6, (Coordinate){16, screenSize.y - BOTTOMMENU_HEIGHT + 5});
    for (i = 0; i < gameSession.activePawn->inventory.torpedoCount; i++) {
        drawhelper_drawSprite(&spriteLibrary.torpedoAnimation[2], (Coordinate){8 + (i * (spriteLibrary.torpedoAnimation->size.x + 1)), screenSize.y - BOTTOMMENU_HEIGHT + 20});
    }

    if (gameSession.activePawn->inventory.carryingFlag) {
        game_drawFlag((Coordinate){gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 1, screenSize.y - 7}, pawn_factionColor(gameSession.activePawn->inventory.flagOfFaction));
    }
}

static void game_drawBottomButtons() {
    Coordinate screenSize = deviceinfo_screenSize();
    RectangleType rect;
    char nextText[] = "next";
    char endText[] = "end turn";
    int startOffsetX = gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 4;
    int startOffsetY = screenSize.y - BOTTOMMENU_HEIGHT + 2;
    int buttonWidth = screenSize.x - startOffsetX - 4;
    int buttonHeight = ((screenSize.y - startOffsetY) / 2) - 2;
    AppColor buttonColor = pawn_factionColor(gameSession.factionTurn) == BELIZEHOLE ? EMERALD : BELIZEHOLE;

    RctSetRectangle(&rect, startOffsetX, startOffsetY, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS);
    FntSetFont(stdFont);
    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(buttonColor);
    drawhelper_drawText(nextText, (Coordinate){startOffsetX + (buttonWidth / 2) - (FntCharsWidth(nextText, StrLen(nextText)) / 2), startOffsetY});
    gameSession.drawingState.barButtonPositions[0] = (Coordinate){startOffsetX, startOffsetY};
    gameSession.drawingState.barButtonHeight = buttonHeight;

    RctSetRectangle(&rect, startOffsetX, startOffsetY + buttonHeight + 2, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS);
    drawhelper_drawText(endText, (Coordinate){startOffsetX + (buttonWidth / 2) - (FntCharsWidth(endText, StrLen(endText)) / 2), startOffsetY + buttonHeight + 2});
    gameSession.drawingState.barButtonPositions[1] = (Coordinate){rect.topLeft.x, rect.topLeft.y};
}

static void game_drawUserInterfaceElements() {
    game_updateMiniMapDrawPosition();
    game_drawBottomBackground();
    game_drawMiniMap();
    if (gameSession.drawingState.shouldDrawButtons && gameSession.movement == NULL && gameSession.attackAnimation == NULL) {
        game_drawBottomButtons();
    } else {
        game_drawBottomActivePawn();
    }
    game_drawBottomActivePawnStats();
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
    game_drawDynamicViews();

    WinSetDrawWindow(screenBuffer);
    RctSetRectangle(&lamerect, 0, 0, screenSize.x, screenSize.y - BOTTOMMENU_HEIGHT);
    WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

    game_drawUserInterfaceElements();

    RctSetRectangle(&lamerect, 0, 0, screenSize.x, screenSize.y);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, GAMEWINDOW_X,
                     GAMEWINDOW_Y,
                     winPaint);
    WinSetDrawWindow(mainWindow);

    if (gameSession.drawingState.requiresPauseAfterLayout) {
        gameSession.drawingState.requiresPauseAfterLayout = false;
        sleep(1000);
    }
}

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t requestMainMenu) {
    gameSession_registerPenInput(eventptr);
    if (eventptr->eType == winDisplayChangedEvent) {
        game_resetForm();
        return true;
    }
    if (eventptr->eType != nilEvent)
        return false;

    gameSession_progressLogic();
    game_drawLayout();
    return true;
}
