#include "game.h"

#include "../platform/i_video.h"
#include "../platform/i_ui.h"
#include "../platform/i_resource.h"
#include "../platform/i_system.h"
#include "../platform/i_input.h"
#include "../platform/i_draw.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "../graphicResources.h"
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

IVideoBuffer *backgroundBuffer = NULL;
IVideoBuffer *overlayBuffer = NULL;
IVideoBuffer *screenBuffer = NULL;
Coordinate lastScreenSize;

static void game_windowCleanup() {
    if (backgroundBuffer != NULL) {
        ivideo_deleteBuffer(backgroundBuffer);
        backgroundBuffer = NULL;
    }
    if (overlayBuffer != NULL) {
        ivideo_deleteBuffer(overlayBuffer);
        overlayBuffer = NULL;
    }
    if (screenBuffer != NULL) {
        ivideo_deleteBuffer(screenBuffer);
        screenBuffer = NULL;
    }
}

static void game_resetForm() {
    IForm *frmP = iui_activeForm();
    Coordinate screenSize = deviceinfo_screenSize();
    IForm *updatedForm = iui_newForm(GAME_FORM, GAME_MENU, screenSize.x, screenSize.y);
    int oldTileSize = hexgrid_tileSize();
    lastScreenSize = screenSize;
    iui_setActiveForm(updatedForm);
    if (frmP != NULL) {
        iui_deleteForm(frmP);
    }
    /* Re-evaluate grid zoom for the (possibly new) screen size. On platforms
       that don't scale the grid this never changes the tile size, so the
       buffers below are left untouched and rendering is unchanged. */
    hexgrid_rescaleIfNeeded();
    if (hexgrid_tileSize() != oldTileSize) {
        game_windowCleanup();
        gameSession.drawingState.shouldRedrawBackground = true;
        gameSession.drawingState.shouldRedrawHeader = true;
    }
    if (gameSession.diaSupport) {
        iui_applyCustomDIAPolicy(updatedForm);
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
    drawhelper_applyTextColor(ASBESTOS);
    drawhelper_applyBackgroundColor(SUNFLOWER);
    if (gameSession.highlightTiles != NULL && gameSession.highlightTileCount > 0) {
        for (i = 0; i < gameSession.highlightTileCount; i++) {
            HighlightTile *tile = &gameSession.highlightTiles[i];
            if (tile->filled) {
                hexgrid_fillTileAtPosition(tile->position, true, tile->color);
                drawhelper_applyForeColor(CLOUDS);
                hexgrid_drawTileAtPosition(tile->position, true);
                if (!gameSession.colorSupport && tile->color == FILLEDTILETYPE_WARNEXCLAMATION) {
                    // draw an exclamation mark in the center so it's obvious that it's a warning tile
                    drawhelper_drawTextCentered("!", viewport_convertedCoordinate(hexgrid_tileCenterPosition(tile->position)), 0, 0);
                }
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

static void game_drawFlag(Coordinate position, AppColor color, Boolean colorSupport, int faction) {
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
    if (!colorSupport) {
        Coordinate factionIndicatorPosition = (Coordinate){position.x - 6, position.y + 1};
        drawhelper_drawSprite(&spriteLibrary.factionIndicator[faction], factionIndicatorPosition);
    }
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

static void game_drawGridItems() {
    int i;
    if (gameSession.level.gridItems == NULL || gameSession.level.gridItemCount <= 0) {
        return;
    }

    for (i = 0; i < gameSession.level.gridItemCount; i++) {
        UInt32 timing = isys_getTicks() / isys_ticksPerSecond() * 2;
        GridItem *gridItem = &gameSession.level.gridItems[i];
        Coordinate center = viewport_convertedCoordinate(hexgrid_tileCenterPosition(gridItem->position));
        ImageSprite *sprite = NULL;
        switch (gridItem->type) {
            case GRIDITEMTYPE_TORPEDOES:
                sprite = &spriteLibrary.torpedoAnimation[gameSession.colorSupport ? 2 : 1];
                break;
            case GRIDITEMTYPE_HEALTH:
                sprite = &spriteLibrary.healthSprite;
                break;
        }

        if (timing % 2 == 0) {
            drawhelper_applyForeColor(pawn_factionColor(timing / 2 % GAMEMECHANICS_MAXPLAYERCOUNT, gameSession.colorSupport));
            drawhelper_drawCircle(center, 7);
        }
        drawhelper_applyForeColor(CLOUDS);
        drawhelper_drawCircle(center, 6);
        hexgrid_drawSpriteAtTile(sprite, gridItem->position, true);
    }
}

static void game_drawActionTiles() {
    int i;
    Char playChar[2];
    IFontID oldFont;
    if (gameSession.level.actionTiles == NULL) {
        return;
    }
    for (i = 0; i < gameSession.level.actionTileCount; i++) {
        ActionTile *actionTile = &gameSession.level.actionTiles[i];
        ImageSprite *sprite;
        Coordinate tileCenterCoordinate = viewport_convertedCoordinate(hexgrid_tileCenterPosition(actionTile->position));
        drawhelper_applyTextColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
        drawhelper_applyForeColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
        oldFont = idraw_setFont(IFONT_LARGEBOLD);
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
                    idraw_setFont(IFONT_SYMBOL);
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
        idraw_restoreFont(oldFont);
    }
}

static void game_drawGridTexts() {
    int i, j;
    IFontID oldFont;
    if (gameSession.level.gridTexts == NULL) {
        return;
    }
    drawhelper_applyTextColor(CLOUDS);
    oldFont = idraw_setFont(IFONT_BOLD);
    for (i = 0; i < gameSession.level.gridTextCount; i++) {
        GridText *gridText = &gameSession.level.gridTexts[i];
        FilledTileType color = gridText->alternateColor ? FILLEDTILETYPE_ATTACK : FILLEDTILETYPE_FEATURED;
        AppColor bgColor = gridText->alternateColor ? ALIZARIN : BELIZEHOLE;
        Char *text = gridText->fixedText;
        if (gridText->simpleText) {
            Coordinate drawPosition = viewport_convertedCoordinate(hexgrid_tileCenterPosition(gridText->position));
            int offset = gridText->position.y % 2 == 0 ? -(hexgrid_tileSize() / 2) : 0;
            drawhelper_applyTextColor(deviceinfo_colorSupported() ? CLOUDS : BELIZEHOLE);
            drawhelper_applyBackgroundColor(DRACULAORCHID);
            drawhelper_drawText(text, (Coordinate){drawPosition.x - hexgrid_tileSize() / 2 + offset + gridText->textOffset.x, drawPosition.y - hexgrid_tileSize() / 2 + gridText->textOffset.y});
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
                /* Badge these letter-tiles as raised buttons (rather than plain
                   outlined hexes) so the NEW/RANK/ABOUT menu reads as deliberate
                   chrome, matching the bevel treatment elsewhere in the UI. */
                hexgrid_drawTileBevelAtPosition(position, true, CLOUDS, DRACULAORCHID);

                drawhelper_drawTextCentered(currChar, (Coordinate){drawPosition.x, drawPosition.y}, 0, 0);
            }
        }
    }
    idraw_restoreFont(oldFont);
}

static void game_drawSceneAnimation() {
    if (gameSession.sceneAnimation == NULL) {
        return;
    }
    drawhelper_drawSprite(gameSession.sceneAnimation->image, gameSession.sceneAnimation->currentPosition);
}

static void game_drawAnimatedStars() {
    int i;
    for (i = 0; i < BACKDROP_ANIMATEDSTARCOUNT - 1; i++) {
        Coordinate coordinate = viewport_convertedCoordinate(gameSession.animatedStarCoordinates[i]);
        drawhelper_drawAnimatedLoopingSprite(spriteLibrary.starAnimation, GFX_FRAMECOUNT_STARANIM, coordinate, 3, i, i % 2 ? 9 : 5);
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
            game_drawFlag(viewport_convertedCoordinate(pawnPosition), pawn_factionColor(pawn->inventory.flagOfFaction, gameSession.colorSupport), gameSession.colorSupport, pawn->inventory.flagOfFaction);
        }
        // Draw faction indicator
        if (!gameSession.colorSupport) {
            Coordinate position = viewport_convertedCoordinate(pawnPosition);
            if (pawn->type == PAWNTYPE_SHIP) {
                position.x += 6;
                position.y -= 8;
            }
            if (pawn->type == PAWNTYPE_SHIP || (pawn->type == PAWNTYPE_BASE && !pawn->inventory.carryingFlag)) {
                drawhelper_drawSprite(&spriteLibrary.factionIndicator[pawn->faction], position);
            }
        }

        if (gameSession_shouldShowHealthBar() && gameSession.factionTurn != gameSession.level.pawns[i].faction) {
            int maxHealthWidth = hexgrid_pawnSize();
            game_drawHealthBar(&gameSession.level.pawns[i], maxHealthWidth, 2, viewport_convertedCoordinate((Coordinate){pawnPosition.x - maxHealthWidth / 2, pawnPosition.y + hexgrid_pawnSize() / 2}));
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

/* The backdrop buffer (game_drawBackground) must cover at least the visible
   viewport, not just the hex grid: on web's zoomed tile sizing a wide
   browser window can be larger than the grid itself, and
   game_drawDynamicViews later copyRect's a viewport-sized region out of that
   buffer - any of that region beyond the buffer's bounds is silently
   skipped, leaving a black void. Shared by game_drawBackdrop/game_drawBackground
   (to size the buffer and its fill) and game_drawStars (to scatter across
   all of it). */
static Coordinate game_backdropSize() {
    Coordinate gridSize = hexgrid_size();
    Coordinate overlaySize = deviceinfo_screenSize();
    overlaySize.y -= BOTTOMMENU_HEIGHT;
    return (Coordinate){
        gridSize.x > overlaySize.x ? gridSize.x : overlaySize.x,
        gridSize.y > overlaySize.y ? gridSize.y : overlaySize.y,
    };
}

static void game_drawBackdrop() {
    int i;
    Coordinate gridSize = hexgrid_size();
    Coordinate backdropSize = game_backdropSize();
    RectangleType rect;
    /* Fill the whole backdrop buffer, not just the hex grid's extent, so a
       viewport wider than the grid doesn't show a black void where the
       buffer was never drawn into. */
    RctSetRectangle(&rect, 0, 0, backdropSize.x, backdropSize.y);
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
    Boolean isGameScreen = gameSession.menuScreenType == MENUSCREEN_GAME;
    int starCount = isGameScreen ? BACKDROP_STARCOUNT : BACKDROP_STARCOUNT * 2;
    /* Off the GAME screen the camera never pans, so scatter across the whole
       backdrop buffer rather than just the hex grid's extent - see
       game_backdropSize(). */
    Coordinate scatterSize = isGameScreen ? hexgrid_size() : game_backdropSize();

    // Draw stars at random locations
    for (i = 0; i < starCount; i++) {
        if (i % 4 == 0) {
            drawhelper_applyForeColor(ASBESTOS);
        } else if (i % 6 == 0) {
            drawhelper_applyForeColor(ALIZARIN);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }

        drawhelper_drawPoint((Coordinate){random(0, scatterSize.x - 1), random(0, scatterSize.y - 1)});
    }
}

static void game_drawGameStartHeader() {
    IFontID oldFont;
    void *resourceHandle;
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
    ImageSprite *shipSprite;
    /* The right decorative group was hardcoded to columns 6/7, which only
       reached the right screen edge because Palm's fixed HEXTILE_SIZE (20)
       and screen width (160) happened to satisfy (7+1)*20 == 160. On web,
       hexgrid_currentTileSize is zoomed to fit the device's actual screen,
       so the column whose center lands at the right edge moves; derive it
       from tileCenterPosition's (col+1)*tileSize relationship instead of
       assuming column 7, mirroring how the left group already sits at the
       left edge via columns -1/0 regardless of tile size. */
    int rightOuterCol = deviceinfo_screenSize().x / hexgrid_tileSize() - 1;
    int rightInnerCol = rightOuterCol - 1;
    Coordinate tilePositions[6];
    int tilePositionsLength = sizeof(tilePositions) / sizeof(Coordinate);
    if (gameSession.menuScreenType == MENUSCREEN_GAME || !gameSession.drawingState.shouldRedrawHeader) {
        return;
    }

    tilePositions[0] = (Coordinate){-1, 0};
    tilePositions[1] = (Coordinate){0, 0};
    tilePositions[2] = (Coordinate){0, 1};
    tilePositions[3] = (Coordinate){rightInnerCol, 0};
    tilePositions[4] = (Coordinate){rightOuterCol, 0};
    tilePositions[5] = (Coordinate){rightOuterCol, 1};

    gameSession.drawingState.shouldRedrawHeader = false;
    ivideo_setDrawTarget(screenBuffer);
    screenSize = deviceinfo_screenSize();

    RctSetRectangle(&rect, 0, 0, screenSize.x, BOTTOMMENU_HEIGHT / 2);
    drawhelper_applyForeColor(headerColorTop);
    drawhelper_fillRectangle(&rect, 0);
    RctSetRectangle(&rect, 0, BOTTOMMENU_HEIGHT / 2, screenSize.x, BOTTOMMENU_HEIGHT / 2);
    drawhelper_applyForeColor(headerColorBottom);
    drawhelper_fillRectangle(&rect, 0);

    /* Bevel the title bar: a light highlight along the very top edge and a
       dark shadow line just above the existing bottom border, so the bar
       reads as a raised panel instead of a flat two-tone stripe. */
    drawhelper_applyForeColor(CLOUDS);
    drawhelper_drawLineBetweenCoordinates((Coordinate){0, 0}, (Coordinate){screenSize.x, 0});
    drawhelper_applyForeColor(headerColorTop);
    drawhelper_drawLineBetweenCoordinates((Coordinate){0, BOTTOMMENU_HEIGHT - 2}, (Coordinate){screenSize.x, BOTTOMMENU_HEIGHT - 2});

    drawhelper_applyTextColor(textColor);
    drawhelper_applyBackgroundColor(centerTileBackgroundColor);
    drawhelper_applyForeColor(tintColor);

    for (i = 0; i < tilePositionsLength; i++) {
        hexgrid_fillTileAtPosition(tilePositions[i], false, filledTileType);
        hexgrid_drawTileAtPosition(tilePositions[i], false);
    }

    switch (random(0, GAMEMECHANICS_MAXPLAYERCOUNT - 1)) {
        case 0:
            shipSprite = &spriteLibrary.shipOneSprite[0];
            break;
        case 1:
            shipSprite = &spriteLibrary.shipTwoSprite[0];
            break;
        case 2:
            shipSprite = &spriteLibrary.shipThreeSprite[0];
            break;
        case 3:
            shipSprite = &spriteLibrary.shipFourSprite[0];
            break;
    }
    hexgrid_drawSpriteAtTile(&shipSprite[0], (Coordinate){0, 0}, false);
    hexgrid_drawSpriteAtTile(&shipSprite[3], (Coordinate){rightInnerCol, 0}, false);

    if (gameSession.menuScreenType == MENUSCREEN_START) {
        // draw version number
        void *versionResourceHandle;
        char *versionText = iresource_loadAppVersion(&versionResourceHandle);
        if (versionText != NULL) {
            drawhelper_drawTextCentered(versionText, hexgrid_tileCenterPosition((Coordinate){rightOuterCol, 1}), 1, -1);
            iresource_releaseAppVersion(versionResourceHandle);
        }
    }

    drawhelper_drawLineBetweenCoordinates((Coordinate){0, BOTTOMMENU_HEIGHT - 1}, (Coordinate){screenSize.x, BOTTOMMENU_HEIGHT - 1});

    RctSetRectangle(&rect, 50, 0, 2, 2);
    drawhelper_applyForeColor(tintColor);
    drawhelper_fillRectangle(&rect, 0);

    RctSetRectangle(&rect, screenSize.x - 52, 0, 2, 2);
    drawhelper_fillRectangle(&rect, 0);

    {
        /* Anchor the title plate to the actual decorative tile columns
           instead of fixed pixel margins, so it stays clear of them when
           hexgrid_currentTileSize is zoomed up for a phone screen instead
           of the historical fixed HEXTILE_SIZE. At the historical fixed
           size this reproduces the same 36px margins as before, so Palm
           OS's layout is unchanged. */
        int tileSize = hexgrid_tileSize();
        int margin = 6;
        Coordinate leftTileCenter = hexgrid_tileCenterPosition((Coordinate){0, 0});
        Coordinate rightTileCenter = hexgrid_tileCenterPosition((Coordinate){rightInnerCol, 0});
        int plateLeft = leftTileCenter.x + tileSize / 2 + margin;
        int plateRight = rightTileCenter.x - tileSize / 2 - margin;
        RctSetRectangle(&rect, plateLeft, 2, plateRight - plateLeft, BOTTOMMENU_HEIGHT - 5);
    }
    drawhelper_fillRectangleWithShadow(&rect, 8, centerTileBackgroundColor, tintColor, false);
    text = iresource_loadString(gameSession_menuTopTitleResource(), &resourceHandle);
    oldFont = idraw_setFont(IFONT_STD);
    centerX = screenSize.x / 2 - idraw_textWidth(text) / 2;
    drawhelper_drawText(text, (Coordinate){centerX, 2});
    iresource_releaseString(resourceHandle);

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
        text = iresource_loadString(gameSession_menuBottomTitleResource(), &resourceHandle);
    }
    if (text != NULL && StrLen(text) > 0) {
        idraw_setFont(IFONT_LARGEBOLD);
        centerX = screenSize.x / 2 - idraw_textWidth(text) / 2;
        drawhelper_drawText(text, (Coordinate){centerX, 12});
        idraw_restoreFont(oldFont);
    }

    if (!gameSession_useValueForBottomTitle() && gameSession.menuScreenType != MENUSCREEN_RANK && gameSession.menuScreenType != MENUSCREEN_RANK_AFTERGAME) {
        iresource_releaseString(resourceHandle);
    }
}

static void game_drawBackground() {
    Coordinate backdropSize;
    if (!gameSession.drawingState.shouldRedrawBackground && backgroundBuffer != NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawBackground = false;
    backdropSize = game_backdropSize();
    if (backgroundBuffer == NULL) {
        backgroundBuffer = ivideo_createBuffer(backdropSize.x, backdropSize.y);
        if (backgroundBuffer == NULL) {
            return;
        }
    }

    ivideo_setDrawTarget(backgroundBuffer);
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
    Coordinate overlaySize = deviceinfo_screenSize();
    overlaySize.y -= BOTTOMMENU_HEIGHT;

    if (overlayBuffer == NULL) {
        overlayBuffer = ivideo_createBuffer(overlaySize.x, overlaySize.y);
    }

    ivideo_setDrawTarget(overlayBuffer);
    if (backgroundBuffer != NULL) {
        ivideo_copyRect(backgroundBuffer, overlayBuffer, gameSession.viewportOffset, overlaySize, (Coordinate){0, 0});
    } else {
        // we don't have enough memory for a background buffer, so draw into the overlayBuffer
        game_drawLowMemBackground(overlaySize);
    }

    game_drawAnimatedStars();
    game_drawHighlightTiles();
    game_drawWarpAndShockwaveAnimation();
    game_drawGridTexts();
    game_drawActionTiles();
    game_drawGridItems();
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
    int pawnSize = hexgrid_pawnSize();
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
    ivideo_copyRect(overlayBuffer, screenBuffer, (Coordinate){pawnCenterPosition.x - pawnSize / 2, pawnCenterPosition.y - pawnSize / 2}, (Coordinate){pawnSize, pawnSize}, targetCenterPosition);

    if (!gameSession.factions[gameSession.factionTurn].human) {  // draw cpu action text
        int textWidth = idraw_textWidth(gameSession.cpuActionText);
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
        game_drawFlag((Coordinate){gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 1, screenSize.y - 7}, pawn_factionColor(gameSession.activePawn->inventory.flagOfFaction, gameSession.colorSupport), gameSession.colorSupport, gameSession.activePawn->inventory.flagOfFaction);
    }
}

static void game_drawBottomButtons() {
    Coordinate screenSize = deviceinfo_screenSize();
    RectangleType rect;
    void *nextResourceHandle;
    void *endResourceHandle;
    char *nextText = iresource_loadString(STRING_NEXT, &nextResourceHandle);
    char *endText = iresource_loadString(STRING_ENDTURN, &endResourceHandle);
    int startOffsetX = gameSession.drawingState.miniMapDrawPosition.x + gameSession.drawingState.miniMapSize.x + 4;
    int startOffsetY = screenSize.y - BOTTOMMENU_HEIGHT + 2;
    int buttonWidth = screenSize.x - startOffsetX - 4;
    int buttonHeight = ((screenSize.y - startOffsetY) / 2) - 2;
    AppColor buttonColor = pawn_factionColor(gameSession.activePawn->faction, gameSession.colorSupport) == BELIZEHOLE ? EMERALD : BELIZEHOLE;

    RctSetRectangle(&rect, startOffsetX, startOffsetY, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS, true);
    idraw_setFont(IFONT_STD);
    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(buttonColor);
    drawhelper_drawText(nextText, (Coordinate){startOffsetX + (buttonWidth / 2) - (idraw_textWidth(nextText) / 2), startOffsetY});
    gameSession.drawingState.barButtonPositions[0] = (Coordinate){startOffsetX, startOffsetY};
    gameSession.drawingState.barButtonHeight = buttonHeight;

    RctSetRectangle(&rect, startOffsetX, startOffsetY + buttonHeight + 2, buttonWidth, buttonHeight);
    drawhelper_fillRectangleWithShadow(&rect, 4, buttonColor, ASBESTOS, true);
    drawhelper_drawText(endText, (Coordinate){startOffsetX + (buttonWidth / 2) - (idraw_textWidth(endText) / 2), startOffsetY + buttonHeight + 2});
    gameSession.drawingState.barButtonPositions[1] = (Coordinate){rect.topLeft.x, rect.topLeft.y};

    iresource_releaseString(nextResourceHandle);
    iresource_releaseString(endResourceHandle);
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
    IVideoBuffer *mainWindow = ivideo_mainScreenBuffer();
    Coordinate screenSize = deviceinfo_screenSize();
    if (screenBuffer == NULL) {
        screenBuffer = ivideo_createBuffer(screenSize.x, screenSize.y);
    }
    game_drawBackground();
    game_drawGameStartHeader();
    game_drawDynamicViews();

    ivideo_setDrawTarget(screenBuffer);
    if (gameSession.menuScreenType == MENUSCREEN_GAME) {
        ivideo_copyRect(overlayBuffer, screenBuffer, (Coordinate){0, 0}, (Coordinate){screenSize.x, screenSize.y - BOTTOMMENU_HEIGHT}, (Coordinate){0, 0});
        game_drawUserInterfaceElements();
    } else {
        ivideo_copyRect(overlayBuffer, screenBuffer, (Coordinate){0, 0}, screenSize, (Coordinate){0, BOTTOMMENU_HEIGHT});
    }

    ivideo_copyRect(screenBuffer, mainWindow, (Coordinate){0, 0}, screenSize, (Coordinate){GAMEWINDOW_X, GAMEWINDOW_Y});
    ivideo_setDrawTarget(mainWindow);

    if (gameSession.drawingState.requiresPauseAfterLayout) {
        gameSession.drawingState.requiresPauseAfterLayout = false;
        deviceinfo_sleep(1000);
    }
}

static Boolean game_checkIfGameIsPaused(IRawEvent *rawEvent) {
    if (iinput_isWindowExitingForm(rawEvent, GAME_FORM)) {
        gameSession.paused = true;
    } else if (iinput_isWindowEnteringForm(rawEvent, GAME_FORM)) {
        gameSession.paused = false;
    }

    return gameSession.paused;
}

Boolean game_mainLoop(IRawEvent *rawEvent, openMainMenuCallback_t requestMainMenu) {
    InputEvent event;
    iinput_translateEvent(rawEvent, &event);
    gameSession_registerPenInput(&event);
    if (event.type == IEVENT_DISPLAYCHANGED) {
        if (iui_activeFormId() == GAME_FORM) {
            game_resetForm();
        } else {
            gameSession.drawingState.shouldResetGameForm = true;
        }

        return true;
    }
    if (gameSession.drawingState.shouldResetGameForm && iui_activeFormId() == GAME_FORM) {
        gameSession.drawingState.shouldResetGameForm = false;
        game_resetForm();
        return true;
    }
    if (event.type == IEVENT_MENU) {
        return gameSession_handleMenu(event.id);
    }
    if (event.type == IEVENT_BUTTON) {
        return gameSession_handleFormButtonTap(event.id);
    }
    if (game_checkIfGameIsPaused(rawEvent)) {
        return false;
    }
    if (event.type != IEVENT_NONE) {
        return false;
    }

    gameSession_progressLogic();
    if (iui_activeFormId() == GAME_FORM) {
        game_drawLayout();
    }

    return true;
}
