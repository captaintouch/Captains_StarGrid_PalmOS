#include "game.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../deviceinfo.h"
#include "../graphicResources.h"
#include "TimeMgr.h"
#include "colors.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "models.h"
#include "pawn.h"
#include "spriteLibrary.h"
#include "viewport.h"

WinHandle backgroundBuffer = NULL;
WinHandle overlayBuffer = NULL;
WinHandle screenBuffer = NULL;
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
    FrmSetMenu(updatedForm, GAME_MENU);
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
        gameSession.drawingState.shouldRedrawBackground = true;
        gameSession.drawingState.shouldRedrawHeader = true;
    }
}

int game_eventDelayTime() {
    return gameSession.paused ? evtWaitForever : 0;
}

void game_setup() {
    spriteLibrary_initialize();
    gameSession_reset(false);
    hexgrid_initialize();
    game_resetForm();
}

void game_cleanup() {
    hexgrid_cleanup();
    game_windowCleanup();
    gameSession_cleanup();
}

static void game_drawWarpAndShockwaveAnimation() {
    int i;
    Coordinate targetPosition;
    AppColor foreColor;
    int *circleDiameter;
    int maskCircleDiameter = 0;
    if (!gameSession.warpAnimation.isWarping && gameSession.shockWaveAnimation == NULL) {
        return;
    }
    if (gameSession.warpAnimation.isWarping) {
        circleDiameter = gameSession.warpAnimation.circleDiameter;
        targetPosition = hexgrid_tileCenterPosition(gameSession.warpAnimation.currentPosition);
        foreColor = BELIZEHOLE;
    } else if (gameSession.shockWaveAnimation != NULL) {
        circleDiameter = gameSession.shockWaveAnimation->circleDiameter;
        targetPosition = hexgrid_tileCenterPosition(gameSession.shockWaveAnimation->basePawn->position);
        foreColor = pawn_factionColor(gameSession.shockWaveAnimation->basePawn->faction, gameSession.colorSupport);
        maskCircleDiameter = gameSession.shockWaveAnimation->maskCircleDiameter;
    } else {
        return;
    }

    for (i = 0; i < WARPCIRCLECOUNT; i++) {
        if (i % 2 == 0) {
            drawhelper_applyForeColor(foreColor);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }
        drawhelper_drawCircle(viewport_convertedCoordinate(targetPosition), circleDiameter[i]);
    }

    if (maskCircleDiameter > 0) {
        // TODO: Create an actual mask
        drawhelper_applyForeColor(DRACULAORCHID);
        drawhelper_drawCircle(viewport_convertedCoordinate(targetPosition), maskCircleDiameter);
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
    if (gameSession.highlightTiles != NULL && gameSession.highlightTileCount > 0) {
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            HighlightTile *tile = &gameSession.highlightTiles[i];
            if (tile->filled) {
                hexgrid_fillTileAtPosition(tile->position, true, tile->color);
                drawhelper_applyForeColor(CLOUDS);
                hexgrid_drawTileAtPosition(tile->position, true);
            } else {
                if (gameSession.colorSupport) {
                    hexgrid_drawTileAtPosition(tile->position, true);
                } else {
                    hexgrid_fillTileAtPosition(tile->position, true, tile->color);
                    drawhelper_applyForeColor(CLOUDS);
                    hexgrid_drawTileAtPosition(tile->position, true);
                }
            }
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
    if (gameSession.colorSupport) {
        return &spriteLibrary_factionShipSprite(pawn->faction)[pawn->orientation];
    } else {
        return &spriteLibrary.shipFourSprite[pawn->orientation];
    }
}

static void game_drawBar(Coordinate position, int width, int height, float barValue, float maxBarValue) {
    RectangleType rect;
    int filledWidth;
    // draw border
    drawhelper_applyForeColor(CLOUDS);
    RctSetRectangle(&rect, position.x - 1, position.y - 1, width + 2, height + 2);
    drawhelper_fillRectangle(&rect, 0);

    filledWidth = ((float)width * barValue) / maxBarValue;
    drawhelper_applyForeColor(ALIZARIN);
    RctSetRectangle(&rect, position.x, position.y, filledWidth, height);
    drawhelper_fillRectangle(&rect, 0);
}

static void game_drawHealthBar(Pawn *pawn, int maxWidth, int height, Coordinate position) {
    int maxHealth;

    if (pawn->type == PAWNTYPE_SHIP) {
        maxHealth = GAMEMECHANICS_MAXSHIPHEALTH;
    } else {
        maxHealth = GAMEMECHANICS_MAXBASEHEALTH;
    }

    game_drawBar(position, maxWidth, height, pawn->inventory.health, maxHealth);
}

static void game_drawActionTiles() {
    int i;
    Char playChar[2];
    FontID oldFont;
    if (gameSession.level.actionTiles == NULL) {
        return;
    }
    for (i = 0; i < gameSession.level.actionTileCount; i++) {
        ActionTile *actionTile = &gameSession.level.actionTiles[i];
        ImageSprite *sprite;
        Coordinate tileCenterCoordinate = viewport_convertedCoordinate(hexgrid_tileCenterPosition(actionTile->position));
        drawhelper_applyTextColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
        drawhelper_applyForeColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
        oldFont = FntSetFont(largeBoldFont);
        if (actionTile->hidden) {
            drawhelper_applyBackgroundColor(DRACULAORCHID);
            drawhelper_applyTextColor(ALIZARIN);
            drawhelper_drawTextCentered("x", tileCenterCoordinate, 0, 0);
        } else {
            if (actionTile->selected) {
                drawhelper_applyBackgroundColor(BELIZEHOLE);
                drawhelper_applyForeColor(deviceinfo_colorSupported() ? CLOUDS : ALIZARIN);
                drawhelper_applyTextColor(CLOUDS);
                hexgrid_fillTileAtPosition(actionTile->position, true, FILLEDTILETYPE_FEATURED);
            } else {
                drawhelper_applyBackgroundColor(DRACULAORCHID);
            }
            switch (actionTile->identifier) {
                case ACTIONTILEIDENTIFIER_HUMANPLAYER:
                    sprite = &spriteLibrary.humanSprite;
                    break;
                case ACTIONTILEIDENTIFIER_CPUPLAYER:
                    sprite = &spriteLibrary.cpuSprite;
                    break;
                case ACTIONTILEIDENTIFIER_LAUNCHGAME:
                case ACTIONTILEIDENTIFIER_ENDGAME:
                case ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS:
                    sprite = NULL;
                    FntSetFont(symbolFont);
                    playChar[0] = 0x04;
                    playChar[1] = '\0';
                    drawhelper_drawTextCentered(playChar, tileCenterCoordinate, 1, 0);
                    break;
                case ACTIONTILEIDENTIFIER_TWOPLAYERS:
                    sprite = NULL;
                    drawhelper_drawTextCentered("2", tileCenterCoordinate, 0, 0);
                    break;
                case ACTIONTILEIDENTIFIER_THREEPLAYERS:
                    sprite = NULL;
                    drawhelper_drawTextCentered("3", tileCenterCoordinate, 0, 0);
                    break;
                case ACTIONTILEIDENTIFIER_FOURPLAYERS:
                    sprite = NULL;
                    drawhelper_drawTextCentered("4", tileCenterCoordinate, 0, 0);
                    break;
            }
            hexgrid_drawTileAtPosition(actionTile->position, true);
            if (sprite != NULL) {
                hexgrid_drawSpriteAtTile(sprite, actionTile->position, true);
            }
        }
        FntSetFont(oldFont);
    }
}

static void game_drawGridTexts() {
    int i, j;
    FontID oldFont;
    if (gameSession.level.gridTexts == NULL) {
        return;
    }
    drawhelper_applyTextColor(CLOUDS);
    oldFont = FntSetFont(boldFont);
    for (i = 0; i < gameSession.level.gridTextCount; i++) {
        GridText *gridText = &gameSession.level.gridTexts[i];
        FilledTileType color = gridText->alternateColor ? FILLEDTILETYPE_ATTACK : FILLEDTILETYPE_FEATURED;
        AppColor bgColor = gridText->alternateColor ? ALIZARIN : BELIZEHOLE;
        Char *text = gridText->fixedText;
        if (gridText->simpleText) {
            Coordinate drawPosition = viewport_convertedCoordinate(hexgrid_tileCenterPosition(gridText->position));
            int offset = gridText->position.y % 2 == 0 ? -(HEXTILE_SIZE / 2) : 0;
            drawhelper_applyTextColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
            drawhelper_applyBackgroundColor(DRACULAORCHID);
            drawhelper_drawText(text, (Coordinate){drawPosition.x - HEXTILE_SIZE / 2 + offset + gridText->textOffset.x, drawPosition.y - HEXTILE_SIZE / 2 + gridText->textOffset.y});
        } else {
            drawhelper_applyBackgroundColor(bgColor);
            for (j = 0; text[j] != '\0'; j++) {
                Coordinate position = (Coordinate){gridText->position.x + j, gridText->position.y};
                Coordinate drawPosition = viewport_convertedCoordinate(hexgrid_tileCenterPosition(position));
                char currChar[2];
                currChar[0] = text[j];
                currChar[1] = '\0';
                hexgrid_fillTileAtPosition(position, true, color);
                drawhelper_applyForeColor(CLOUDS);
                hexgrid_drawTileAtPosition(position, true);

                drawhelper_drawTextCentered(currChar, (Coordinate){drawPosition.x, drawPosition.y}, 0, 0);
            }
        }
    }
    FntSetFont(oldFont);
}

static void game_drawSceneAnimation() {
    if (gameSession.sceneAnimation == NULL) {
        return;
    }
    drawhelper_drawSprite(gameSession.sceneAnimation->image, gameSession.sceneAnimation->currentPosition);
}

static void game_drawAnimatedStars() {
    int i;
    for (i = 0; i < BACKDROP_ANIMATEDSTARCOUNT; i++) {
        Coordinate coordinate = viewport_convertedCoordinate(gameSession.animatedStarCoordinates[i]);
        drawhelper_drawAnimatedLoopingSprite(spriteLibrary.starAnimation, GFX_FRAMECOUNT_STARANIM, coordinate, 3, i, 5);
    }
}

static void game_drawPawns() {
    int i, j;
    if (gameSession.activePawn != NULL && gameSession.menuScreenType == MENUSCREEN_GAME) {
        drawhelper_applyForeColor(EMERALD);
        if (gameSession.colorSupport) {
            hexgrid_drawTileAtPosition(gameSession.activePawn->position, true);
        } else {
            hexgrid_fillTileAtPosition(gameSession.activePawn->position, true, FILLEDTILETYPE_MOVE);
        }
    }

    // DRAW BASES
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        AppColor baseActivityColor;
        Pawn *pawn = &gameSession.level.pawns[i];
        Coordinate pawnPosition, pawnPositionConverted;
        RectangleType rect;
        if (pawn->type != PAWNTYPE_BASE || isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            continue;
        }
        pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        pawnPositionConverted = viewport_convertedCoordinate(pawnPosition);
        RctSetRectangle(&rect, pawnPositionConverted.x - 6, pawnPositionConverted.y - 6, 12, 12);
        drawhelper_applyForeColor(pawn_factionColor(pawn->faction, gameSession.colorSupport));
        drawhelper_fillRectangle(&rect, 0);
        hexgrid_drawSpriteAtTile(&spriteLibrary.baseSprite, pawn->position, true);

        // Draw base activity indicator
        baseActivityColor = pawn_baseActivityIndicatorColor(pawn, gameSession.colorSupport, gameSession.currentTurn);
        pawnPositionConverted = viewport_convertedCoordinate((Coordinate){pawnPosition.x + 7, pawnPosition.y - 6});
        drawhelper_applyForeColor(ASBESTOS);
        drawhelper_drawCircle(pawnPositionConverted, 4);
        drawhelper_applyForeColor(baseActivityColor);
        drawhelper_drawCircle(pawnPositionConverted, 3);
    }

    // DRAW SHIPS
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        Pawn *pawn = &gameSession.level.pawns[i];
        Coordinate pawnPosition;
        ImageSprite *shipSprite;
        Boolean didDrawSprite = false;
        if (pawn->type != PAWNTYPE_SHIP || isInvalidCoordinate(gameSession.level.pawns[i].position)) {
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
            didDrawSprite = true;
        } else if (gameSession.shockWaveAnimation != NULL) {
            for (j = 0; j < gameSession.shockWaveAnimation->affectedPawnCount; j++) {
                if (gameSession.shockWaveAnimation->affectedPawnIndices[j] == i) {
                    drawhelper_drawSprite(shipSprite, viewport_convertedCoordinate(gameSession.shockWaveAnimation->pawnIntermediatePositions[j]));
                    didDrawSprite = true;
                    break;
                }
            }
        }

        if (!didDrawSprite) {
            hexgrid_drawSpriteAtTile(shipSprite, pawnPosition, true);
        }
    }

    // DRAW ACCESSORIES (FLAGS, FACTION INDICATORS)
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        Pawn *pawn = &gameSession.level.pawns[i];
        Coordinate pawnPosition = hexgrid_tileCenterPosition(pawn->position);
        if (isInvalidCoordinate(pawn->position)) {
            continue;
        }
        if (pawn->inventory.carryingFlag) {
            game_drawFlag(viewport_convertedCoordinate(pawnPosition), pawn_factionColor(pawn->inventory.flagOfFaction, gameSession.colorSupport));
        }
        // Draw faction indicator
        if (!gameSession.colorSupport) {
            Coordinate position = viewport_convertedCoordinate(pawnPosition);
            if (pawn->type == PAWNTYPE_SHIP) {
                position.x += 6;
                position.y -= 8;
            }
            drawhelper_drawSprite(&spriteLibrary.factionIndicator[pawn->faction], position);
        }

        if (gameSession_shouldShowHealthBar() && gameSession.factionTurn != gameSession.level.pawns[i].faction) {
            int maxHealthWidth = HEXTILE_PAWNSIZE;
            game_drawHealthBar(&gameSession.level.pawns[i], maxHealthWidth, 2, viewport_convertedCoordinate((Coordinate){pawnPosition.x - maxHealthWidth / 2, pawnPosition.y + HEXTILE_PAWNSIZE / 2}));
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
    bottomMenu_display(gameSession.displayButtons, gameSession.displayButtonCount, gameSession.colorSupport);
}

static void game_drawBackdrop() {
    int i;
    Coordinate gridSize = hexgrid_size();
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, gridSize.x, gridSize.y);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 0);
    if (deviceinfo_colorSupported()) {
        ImageSprite nebulaSprite = spriteLibrary_nebulaSprite();
        if (gameSession.menuScreenType == MENUSCREEN_GAME) {
            for (i = 0; i < 8; i++) {
                drawhelper_drawSprite(&nebulaSprite, (Coordinate){random(-20, gridSize.x - 50), random(-20, gridSize.y - 50)});
            }
        } else {
            drawhelper_drawSprite(&nebulaSprite, (Coordinate){20, 20});
        }
        drawhelper_releaseImage(nebulaSprite.imageData);
    }
}

static void game_drawStars() {
    int i;
    int starCount = gameSession.menuScreenType == MENUSCREEN_GAME ? BACKDROP_STARCOUNT : BACKDROP_STARCOUNT * 2;
    Coordinate gridSize = hexgrid_size();

    // Draw stars at random locations
    for (i = 0; i < starCount; i++) {
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

static void game_drawGameStartHeader() {
    FontID oldFont;
    MemHandle resourceHandle;
    RectangleType rect;
    char *text;
    Coordinate screenSize;
    int centerX;
    int i;
    char fixedText[20];
    AppColor centerTileBackgroundColor = BELIZEHOLE;
    AppColor tintColor = CLOUDS;
    FilledTileType filledTileType = FILLEDTILETYPE_FEATURED;
    AppColor headerColorTop = DRACULAORCHID;
    AppColor headerColorBottom = BELIZEHOLE;
    AppColor textColor = CLOUDS;
    Coordinate tilePositions[] = {
        (Coordinate){-1, 0},
        (Coordinate){0, 0},
        (Coordinate){0, 1},
        (Coordinate){6, 0},
        (Coordinate){7, 0},
        (Coordinate){7, 1},
    };
    int tilePositionsLength = sizeof(tilePositions) / sizeof(Coordinate);
    if (gameSession.menuScreenType == MENUSCREEN_GAME || !gameSession.drawingState.shouldRedrawHeader) {
        return;
    }

    gameSession.drawingState.shouldRedrawHeader = false;
    WinSetDrawWindow(screenBuffer);
    screenSize = deviceinfo_screenSize();

    RctSetRectangle(&rect, 0, 0, screenSize.x, BOTTOMMENU_HEIGHT / 2);
    drawhelper_applyForeColor(headerColorTop);
    drawhelper_fillRectangle(&rect, 0);
    RctSetRectangle(&rect, 0, BOTTOMMENU_HEIGHT / 2, screenSize.x, BOTTOMMENU_HEIGHT / 2);
    drawhelper_applyForeColor(headerColorBottom);
    drawhelper_fillRectangle(&rect, 0);

    drawhelper_applyTextColor(textColor);
    drawhelper_applyBackgroundColor(centerTileBackgroundColor);
    drawhelper_applyForeColor(tintColor);

    for (i = 0; i < tilePositionsLength; i++) {
        hexgrid_fillTileAtPosition(tilePositions[i], false, filledTileType);
        hexgrid_drawTileAtPosition(tilePositions[i], false);
    }
    hexgrid_drawSpriteAtTile(&spriteLibrary.shipFourSprite[0], (Coordinate){0, 0}, false);
    hexgrid_drawSpriteAtTile(&spriteLibrary.shipFourSprite[3], (Coordinate){6, 0}, false);

    drawhelper_drawLineBetweenCoordinates((Coordinate){0, BOTTOMMENU_HEIGHT - 1}, (Coordinate){screenSize.x, BOTTOMMENU_HEIGHT - 1});

    RctSetRectangle(&rect, 50, 0, 2, 2);
    drawhelper_applyForeColor(tintColor);
    drawhelper_fillRectangle(&rect, 0);

    RctSetRectangle(&rect, screenSize.x - 52, 0, 2, 2);
    drawhelper_fillRectangle(&rect, 0);

    RctSetRectangle(&rect, 36, 2, screenSize.x - 72, BOTTOMMENU_HEIGHT - 5);
    drawhelper_fillRectangleWithShadow(&rect, 8, centerTileBackgroundColor, tintColor, false);
    resourceHandle = DmGetResource(strRsc, gameSession_menuTopTitleResource());
    text = (char *)MemHandleLock(resourceHandle);
    oldFont = FntSetFont(stdFont);
    centerX = screenSize.x / 2 - FntCharsWidth(text, StrLen(text)) / 2;
    drawhelper_drawText(text, (Coordinate){centerX, 2});
    MemHandleUnlock(resourceHandle);
    DmReleaseResource(resourceHandle);

    if (gameSession.menuScreenType == MENUSCREEN_RANK || gameSession.menuScreenType == MENUSCREEN_RANK_AFTERGAME) {
        int barWidth = 55;
        int currValue = gameSession_menuTopTitleResource() - STRING_RANK0;
        int maxValue = RANK_COUNT;
        game_drawBar((Coordinate){screenSize.x / 2 - barWidth / 2, 17}, barWidth, 8, fmax(1, currValue), maxValue);
        text = NULL;
    } else if (gameSession_useValueForBottomTitle()) {
        StrIToA(fixedText, gameSession_valueForBottomTitle());
        text = fixedText;
    } else {
        resourceHandle = DmGetResource(strRsc, gameSession_menuBottomTitleResource());
        text = (char *)MemHandleLock(resourceHandle);
    }
    if (text != NULL && StrLen(text) > 0) {
        FntSetFont(largeBoldFont);
        centerX = screenSize.x / 2 - FntCharsWidth(text, StrLen(text)) / 2;
        drawhelper_drawText(text, (Coordinate){centerX, 12});
        FntSetFont(oldFont);
    }

    if (!gameSession_useValueForBottomTitle() && gameSession.menuScreenType != MENUSCREEN_RANK && gameSession.menuScreenType != MENUSCREEN_RANK_AFTERGAME) {
        MemHandleUnlock(resourceHandle);
        DmReleaseResource(resourceHandle);
    }
}

static void game_drawBackground() {
    Err err = errNone;
    Coordinate gridSize;
    if (!gameSession.drawingState.shouldRedrawBackground && backgroundBuffer != NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawBackground = false;
    gridSize = hexgrid_size();
    if (backgroundBuffer == NULL) {
        backgroundBuffer = WinCreateOffscreenWindow(gridSize.x, gridSize.y, nativeFormat, &err);
        if (err != errNone) {
            backgroundBuffer = NULL;
            return;
        }
    }

    WinSetDrawWindow(backgroundBuffer);
    game_drawBackdrop();
    if (gameSession.menuScreenType == MENUSCREEN_GAME) {
        hexgrid_drawEntireGrid(false);
    }
    game_drawStars();
}

static void game_drawLowMemBackground(Coordinate screenSize) {
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, screenSize.x, screenSize.y);
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 0);
    if (!gameSession.drawingState.awaitingEndMiniMapScrolling && !gameSession_animating() && gameSession.state != GAMESTATE_CHOOSEPAWNACTION && gameSession.factions[gameSession.factionTurn].human && gameSession.menuScreenType == MENUSCREEN_GAME) {
        hexgrid_drawEntireGrid(true);
    }
}

static void game_drawDynamicViews() {  // ships, special tiles, etc.
    // everything drawn in this function must have it's coordinates offset to the current viewport
    RectangleType lamerect;
    Err err = errNone;
    Coordinate overlaySize = deviceinfo_screenSize();
    overlaySize.y -= BOTTOMMENU_HEIGHT;

    if (overlayBuffer == NULL) {
        overlayBuffer = WinCreateOffscreenWindow(overlaySize.x, overlaySize.y, nativeFormat, &err);
    }

    WinSetDrawWindow(overlayBuffer);
    RctSetRectangle(&lamerect, gameSession.viewportOffset.x, gameSession.viewportOffset.y, overlaySize.x, overlaySize.y);
    if (backgroundBuffer != NULL) {
        WinCopyRectangle(backgroundBuffer, overlayBuffer, &lamerect, 0, 0, winPaint);
    } else {
        // we don't have enough memory for a background buffer, so draw into the overlayBuffer
        game_drawLowMemBackground(overlaySize);
    }

    game_drawAnimatedStars();
    game_drawHighlightTiles();
    game_drawWarpAndShockwaveAnimation();
    game_drawGridTexts();
    game_drawActionTiles();
    game_drawPawns();
    game_drawSceneAnimation();
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
    minimap_draw(gameSession.level.pawns,
                 gameSession.level.pawnCount,
                 gameSession.drawingState.miniMapDrawPosition,
                 gameSession.drawingState.miniMapSize,
                 gameSession.movement,
                 gameSession.activePawn,
                 gameSession.viewportOffset,
                 gameSession.colorSupport);
}

static void game_drawBottomBackground() {
    Coordinate screenSize = deviceinfo_screenSize();
    int width = (float)screenSize.x * 0.4;
    int centerOffsetX = (screenSize.x - width) / 2;
    RectangleType rect;
    RctSetRectangle(&rect, 0, screenSize.y - BOTTOMMENU_HEIGHT, screenSize.x, BOTTOMMENU_HEIGHT);
    drawhelper_applyForeColor(pawn_factionColor(gameSession.activePawn->faction, gameSession.colorSupport));
    drawhelper_fillRectangle(&rect, 0);
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
        drawhelper_applyTextColor(gameSession.colorSupport ? CLOUDS : ASBESTOS);
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
        game_drawFlag((Coordinate){gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 1, screenSize.y - 7}, pawn_factionColor(gameSession.activePawn->inventory.flagOfFaction, gameSession.colorSupport));
    }
}

static void game_drawBottomButtons() {
    Coordinate screenSize = deviceinfo_screenSize();
    RectangleType rect;
    MemHandle nextResourceHandle = DmGetResource(strRsc, STRING_NEXT);
    MemHandle endResourceHandle = DmGetResource(strRsc, STRING_ENDTURN);
    char *nextText = (char *)MemHandleLock(nextResourceHandle);
    char *endText = (char *)MemHandleLock(endResourceHandle);
    int startOffsetX = gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 4;
    int startOffsetY = screenSize.y - BOTTOMMENU_HEIGHT + 2;
    int buttonWidth = screenSize.x - startOffsetX - 4;
    int buttonHeight = ((screenSize.y - startOffsetY) / 2) - 2;
    AppColor buttonColor = pawn_factionColor(gameSession.activePawn->faction, gameSession.colorSupport) == BELIZEHOLE ? EMERALD : BELIZEHOLE;

    RctSetRectangle(&rect, startOffsetX, startOffsetY, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS, true);
    FntSetFont(stdFont);
    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(buttonColor);
    drawhelper_drawText(nextText, (Coordinate){startOffsetX + (buttonWidth / 2) - (FntCharsWidth(nextText, StrLen(nextText)) / 2), startOffsetY});
    gameSession.drawingState.barButtonPositions[0] = (Coordinate){startOffsetX, startOffsetY};
    gameSession.drawingState.barButtonHeight = buttonHeight;

    RctSetRectangle(&rect, startOffsetX, startOffsetY + buttonHeight + 2, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS, true);
    drawhelper_drawText(endText, (Coordinate){startOffsetX + (buttonWidth / 2) - (FntCharsWidth(endText, StrLen(endText)) / 2), startOffsetY + buttonHeight + 2});
    gameSession.drawingState.barButtonPositions[1] = (Coordinate){rect.topLeft.x, rect.topLeft.y};

    MemHandleUnlock(nextResourceHandle);
    DmReleaseResource(nextResourceHandle);
    MemHandleUnlock(endResourceHandle);
    DmReleaseResource(endResourceHandle);
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
        screenBuffer = WinCreateOffscreenWindow(screenSize.x, screenSize.y, nativeFormat, &err);
    }
    game_drawBackground();
    game_drawGameStartHeader();
    game_drawDynamicViews();

    WinSetDrawWindow(screenBuffer);
    if (gameSession.menuScreenType == MENUSCREEN_GAME) {
        RctSetRectangle(&lamerect, 0, 0, screenSize.x, screenSize.y - BOTTOMMENU_HEIGHT);
        WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, 0, winPaint);
        game_drawUserInterfaceElements();
    } else {
        RctSetRectangle(&lamerect, 0, 0, screenSize.x, screenSize.y);
        WinCopyRectangle(overlayBuffer, screenBuffer, &lamerect, 0, BOTTOMMENU_HEIGHT, winPaint);
    }

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

static Boolean game_checkIfGameIsPaused(EventType *eventptr) {
    if (eventptr->eType == winExitEvent) {
        if (eventptr->data.winExit.exitWindow ==
            (WinHandle)FrmGetFormPtr(GAME_FORM)) {
            gameSession.paused = true;
        }
    } else if (eventptr->eType == winEnterEvent) {
        if (eventptr->data.winEnter.enterWindow ==
                (WinHandle)FrmGetFormPtr(GAME_FORM) &&
            eventptr->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm()) {
            gameSession.paused = false;
        }
    }

    return gameSession.paused;
}

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t requestMainMenu) {
    gameSession_registerPenInput(eventptr);
    if (eventptr->eType == winDisplayChangedEvent) {
        if (FrmGetActiveFormID() == GAME_FORM) {
            game_resetForm();
        } else {
            gameSession.drawingState.shouldResetGameForm = true;
        }

        return true;
    }
    if (gameSession.drawingState.shouldResetGameForm && FrmGetActiveFormID() == GAME_FORM) {
        gameSession.drawingState.shouldResetGameForm = false;
        game_resetForm();
        return true;
    }
    if ((eventptr->eType == menuEvent)) {
        return gameSession_handleMenu(eventptr->data.menu.itemID);
    }
    if ((eventptr->eType == ctlSelectEvent)) {
        return gameSession_handleFormButtonTap(eventptr->data.ctlSelect.controlID);
    }
    if (game_checkIfGameIsPaused(eventptr)) {
        return false;
    }
    if (eventptr->eType != nilEvent) {
        return false;
    }

    gameSession_progressLogic();
    if (FrmGetActiveFormID() == GAME_FORM) {
        game_drawLayout();
    }

    return true;
}
