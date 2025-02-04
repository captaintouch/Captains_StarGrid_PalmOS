#include "gamesession.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "movement.h"
#include "viewport.h"
#include "pawnActionMenuViewModel.h"
#include "models.h"

void gameSession_initialize() {
    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.lastPenInput.moving = false;
    gameSession.lastPenInput.touchCoordinate = (Coordinate){-1, -1};
    gameSession.lastPenInput.wasUpdatedFlag = false;

    gameSession.diaSupport = deviceinfo_diaSupported();
    gameSession.colorSupport = deviceinfo_colorSupported();

    // set PAWNS
    gameSession.pawnCount = 5;
    gameSession.pawns = MemPtrNew(sizeof(Pawn) * gameSession.pawnCount) ;
    MemSet(gameSession.pawns, (sizeof(Pawn) * gameSession.pawnCount), 0);
    gameSession.pawns[0] = (Pawn){(Coordinate){2, 3}, 0, false, PAWNTYPE_SHIP, 0};
    gameSession.pawns[1] = (Pawn){(Coordinate){5, 4}, 0, false, PAWNTYPE_SHIP, 0};
    gameSession.pawns[2] = (Pawn){(Coordinate){1, 4}, 0, false, PAWNTYPE_SHIP, 1};
    gameSession.pawns[3] = (Pawn){(Coordinate){8, 8}, 0, false, PAWNTYPE_FLAG, 0};
    gameSession.pawns[4] = (Pawn){(Coordinate){1, 1}, 0, false, PAWNTYPE_FLAG, 1};

    gameSession.activePawn = &gameSession.pawns[0];

    gameSession.drawingState = (DrawingState){true, true, (Coordinate){0, 0}};
    gameSession.highlightTiles = NULL;
    gameSession.highlightTileCount = 0;
    gameSession.secondaryHighlightTiles = NULL;
    gameSession.secondaryHighlightTileCount = 0;

    gameSession.attackAnimation = NULL;
    gameSession.movement = NULL;

    gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;

    gameSession.viewportOffset = (Coordinate){0, 0};
}

void gameSession_registerPenInput(EventPtr eventptr) {
    inputPen_updateEventDetails(&gameSession.lastPenInput, eventptr);
}

static Coordinate gameSession_validViewportOffset(Coordinate position) {
    Coordinate newOffset;
    Coordinate screenSize = deviceinfo_screenSize();
    int gameWindowHeight = screenSize.y - BOTTOMMENU_HEIGHT;
    Coordinate gridSize = hexgrid_size();
    newOffset.x = fmin(gridSize.x - screenSize.x + 1, fmax(0, position.x - screenSize.x / 2));
    newOffset.y = fmin(gridSize.y - gameWindowHeight + 1, fmax(0, position.y - gameWindowHeight / 2));
    return newOffset;
}

static void gameSession_updateViewPortOffset(Boolean forceUpdateActivePawn) {
    Coordinate position;
    if (gameSession.activePawn != NULL && forceUpdateActivePawn) {
        position = hexgrid_tileCenterPosition(gameSession.activePawn->position);
    } else if (gameSession.movement != NULL) {
        position = gameSession.movement->pawnPosition;
    } else {
        return;
    }
    gameSession.viewportOffset = gameSession_validViewportOffset(position);
}

static Pawn *gameSession_pawnAtTile(Coordinate tile) {
    int i;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].position.x == tile.x && gameSession.pawns[i].position.y == tile.y) {
            return &gameSession.pawns[i];
        }
    }
    return NULL;
}

static int gameSession_maxRange(TargetSelectionType targetSelectionType) {
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            return GAMEMECHANICS_MAXTILEMOVERANGE;
        case TARGETSELECTIONTYPE_PHASER:
            return GAMEMECHANICS_MAXTILEPHASERRANGE;
        case TARGETSELECTIONTYPE_TORPEDO:
            return GAMEMECHANICS_MAXTILETORPEDORANGE;
    }
}

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition, TargetSelectionType targetSelectionType) {
    int i;
    int maxTileRange = gameSession_maxRange(targetSelectionType);
    Coordinate *coordinates = NULL;
    int coordinatesCount = 0;
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            coordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.pawnCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (gameSession.pawns[i].pawnType != PAWNTYPE_SHIP) {
                    continue;
                }
                coordinates[coordinatesCount] = gameSession.pawns[i].position;
                coordinatesCount++;
            }
            MemPtrResize(coordinates, sizeof(Coordinate) * coordinatesCount);
            movement_findTilesInRange(currentPosition, maxTileRange, coordinates, coordinatesCount, &gameSession.highlightTiles, &gameSession.highlightTileCount);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            gameSession.highlightTiles = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.pawnCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (gameSession.pawns[i].faction == gameSession.activePawn->faction || movement_distance(gameSession.pawns[i].position, currentPosition) > maxTileRange) {
                    continue;
                }
                gameSession.highlightTiles[coordinatesCount] = gameSession.pawns[i].position;
                coordinatesCount++;
            }
            coordinatesCount++;
            gameSession.highlightTileCount = coordinatesCount;
            movement_findTilesInRange(currentPosition, maxTileRange, NULL, 0, &gameSession.secondaryHighlightTiles, &gameSession.secondaryHighlightTileCount);
            break;
    }

    
    if (coordinates != NULL) {
        MemPtrFree(coordinates);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_showPawnActions() {
    pawnActionMenuViewModel_setupMenuForPawn(gameSession.activePawn, &gameSession.displayButtons, &gameSession.displayButtonCount);
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.state = GAMESTATE_CHOOSEPAWNACTION;
}

static Boolean gameSession_handleTileTap() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (selectedPawn != NULL) {
        gameSession.activePawn = selectedPawn;
        gameSession_updateViewPortOffset(true);
        gameSession_showPawnActions();
        return true;
    }
    return false;
}

static Boolean gameSession_handleMiniMapTap() {
    Coordinate viewportOffset = minimap_viewportOffsetForTap(gameSession.lastPenInput.touchCoordinate, gameSession.drawingState.miniMapDrawPosition, gameSession.drawingState.miniMapSize);
    if (isInvalidCoordinate(viewportOffset)) {
        return false;
    }
    gameSession.viewportOffset = gameSession_validViewportOffset(viewportOffset);
    return true;
}

static Boolean gameSession_highlightTilesContains(Coordinate coordinate) {
    int i;
    for (i = 0; i < gameSession.highlightTileCount; i++) {
        if (gameSession.highlightTiles[i].x == coordinate.x && gameSession.highlightTiles[i].y == coordinate.y) {
            return true;
        }
    }
    return false;
}

static void gameSession_clearAttack() {
    if (gameSession.attackAnimation != NULL) {
        if (gameSession.attackAnimation->lines != NULL) {
            MemPtrFree(gameSession.attackAnimation->lines);
            gameSession.attackAnimation->lines = NULL;
        }
        MemPtrFree(gameSession.attackAnimation);
        gameSession.attackAnimation = NULL;
    }
}

static void gameSession_clearMovement() {
    if (gameSession.movement != NULL) {
        if (gameSession.movement->trajectory.tileCoordinates != NULL) {
            MemPtrFree(gameSession.movement->trajectory.tileCoordinates);
            gameSession.movement->trajectory.tileCoordinates = NULL;
        }
        MemPtrFree(gameSession.movement);
        gameSession.movement = NULL;
    }
}

static void gameSession_resetHighlightTiles() {
    if (gameSession.highlightTiles != NULL) {
        MemPtrFree(gameSession.highlightTiles);
        gameSession.highlightTiles = NULL;
        gameSession.highlightTileCount = 0;
    }
    if (gameSession.secondaryHighlightTiles != NULL) {
        MemPtrFree(gameSession.secondaryHighlightTiles);
        gameSession.secondaryHighlightTiles = NULL;
        gameSession.secondaryHighlightTileCount = 0;
    }
}

static void gameSession_handleTargetSelection() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    if (!gameSession_highlightTilesContains(selectedTile)) {
        gameSession_resetHighlightTiles();
        gameSession.state = GAMESTATE_DEFAULT;
        gameSession.drawingState.shouldRedrawOverlay = true;
        return;
    }

    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            gameSession_clearMovement();
            gameSession.movement = (Movement *)MemPtrNew(sizeof(Movement));
            MemSet(gameSession.movement, sizeof(Movement), 0);
            gameSession.movement->launchTimestamp = TimGetTicks();
            gameSession.movement->trajectory = movement_trajectoryBetween((Coordinate){gameSession.activePawn->position.x, gameSession.activePawn->position.y}, selectedTile);
            gameSession.movement->pawn = gameSession.activePawn;
            gameSession.activePawn->position = selectedTile;
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            gameSession_clearAttack();
            if (gameSession_pawnAtTile(selectedTile) != NULL) {
                gameSession.attackAnimation = (AttackAnimation *)MemPtrNew(sizeof(AttackAnimation));
                MemSet(gameSession.attackAnimation, sizeof(AttackAnimation), 0);
                gameSession.attackAnimation->launchTimestamp = TimGetTicks();
                gameSession.attackAnimation->target = selectedTile;
                gameSession.activePawn->orientation = movement_orientationBetween(gameSession.activePawn->position, selectedTile);
            } else {
                gameSession.state = GAMESTATE_DEFAULT;
            }
            break;
    }

    gameSession_resetHighlightTiles();
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_handlePawnActionButtonSelection() {
    int i;
    int selectedIndex = bottomMenu_selectedIndex(gameSession.lastPenInput.touchCoordinate);

    if (selectedIndex >= gameSession.displayButtonCount) {
        return;
    }

    switch (pawnActionMenuViewModel_actionAtIndex(selectedIndex, gameSession.activePawn)) {
        case MenuActionTypeCancel:
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case MenuActionTypeCloak:
            gameSession.activePawn->cloaked = !gameSession.activePawn->cloaked;
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case MenuActionTypeTorpedo:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_TORPEDO;
            break;
        case MenuActionTypePhaser:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_PHASER;
            break;
        case MenuActionTypeMove:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;
            break;
    }

    for (i = 0; i < gameSession.displayButtonCount; i++) {
        MemPtrFree(gameSession.displayButtons[i].text);
    }
    MemPtrFree(gameSession.displayButtons);
    gameSession.displayButtons = NULL;
    gameSession.displayButtonCount = 0;

    if (gameSession.state == GAMESTATE_SELECTTARGET) {
        gameSession_updateValidPawnPositionsForMovement(gameSession.activePawn->position, gameSession.targetSelectionType);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

AppColor gameSession_hightlightTilesColor() {
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            return EMERALD;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            return ALIZARIN;
    }
}

static Coordinate gameSession_getBoxCoordinate(Coordinate center, float t, int boxSize) {
    int halfSize = boxSize / 2;
    int perimeter = 4 * boxSize;
    int pos = (int)(t * perimeter) % perimeter;
    
    Coordinate result = {center.x, center.y};
    
    if (pos < boxSize) {
        result.x = center.x - halfSize + pos;
        result.y = center.y - halfSize;
    } else if (pos < 2 * boxSize) {
        result.x = center.x + halfSize;
        result.y = center.y - halfSize + (pos - boxSize);
    } else if (pos < 3 * boxSize) {
        result.x = center.x + halfSize - (pos - 2 * boxSize);
        result.y = center.y + halfSize;
    } else {
        result.x = center.x - halfSize;
        result.y = center.y + halfSize - (pos - 3 * boxSize);
    }
    
    return result;
}

static void gameSession_progressUpdateAttack() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    Coordinate targetCenter;
    Line attackLine;
    if (gameSession.attackAnimation == NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * 1.5);

    if (gameSession.attackAnimation->lines != NULL) {
        MemPtrFree(gameSession.attackAnimation->lines);
    }

    targetCenter = hexgrid_tileCenterPosition(gameSession.attackAnimation->target);
    attackLine = (Line){hexgrid_tileCenterPosition(gameSession.activePawn->position), gameSession_getBoxCoordinate(targetCenter, timePassedScale, HEXTILE_PAWNSIZE / 3)};
    gameSession.attackAnimation->lines = (Line *)MemPtrNew(sizeof(Line) * 3);
    gameSession.attackAnimation->lines[0] = (Line){attackLine.startpoint, movement_coordinateAtPercentageOfLine(attackLine, remapToMax(timePassedScale * 2.4, 1))};
    attackLine.startpoint = gameSession.attackAnimation->lines[0].endpoint;
    gameSession.attackAnimation->lines[1] = (Line){gameSession.attackAnimation->lines[0].endpoint, movement_coordinateAtPercentageOfLine(attackLine, 0.7)};
    gameSession.attackAnimation->lines[2] = (Line){gameSession.attackAnimation->lines[1].endpoint, attackLine.endpoint};
    gameSession.attackAnimation->lineCount = 3;

    if (timePassedScale >= 1) {
        gameSession_clearAttack();
        gameSession.state = GAMESTATE_DEFAULT;
    }
}

static void gameSession_progressUpdateMovement() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    if (gameSession.movement == NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;

    timeSinceLaunch = TimGetTicks() - gameSession.movement->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * ((float)gameSession.movement->trajectory.tileCount - 1) / 1.7);
    gameSession.movement->pawnPosition = movement_coordinateAtPercentageOfTrajectory(gameSession.movement->trajectory, timePassedScale, &gameSession.movement->pawn->orientation);
    gameSession_updateViewPortOffset(false);

    if (timePassedScale >= 1) {
        gameSession_clearMovement();
        gameSession_updateViewPortOffset(true);
        gameSession.state = GAMESTATE_DEFAULT;
    }
}

void gameSession_progressLogic() {
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        // Handle pen input
        gameSession.lastPenInput.wasUpdatedFlag = false;

        if (gameSession.lastPenInput.moving) {
            switch (gameSession.state) {
                case GAMESTATE_DEFAULT:
                    gameSession_handleMiniMapTap();
                    break;
                case GAMESTATE_CHOOSEPAWNACTION:
                case GAMESTATE_SELECTTARGET:
                    break;
            }
        } else {
            switch (gameSession.state) {
                case GAMESTATE_DEFAULT:
                    if (gameSession_handleMiniMapTap()) break;
                    if (gameSession_handleTileTap()) break;
                    break;
                case GAMESTATE_CHOOSEPAWNACTION:
                    gameSession_handlePawnActionButtonSelection();
                    break;
                case GAMESTATE_SELECTTARGET:
                    gameSession_handleTargetSelection();
                    break;
            }
        }
    }
    gameSession_progressUpdateMovement();
    gameSession_progressUpdateAttack();
}
