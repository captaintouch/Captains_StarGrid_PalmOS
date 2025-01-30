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

void gameSession_initialize() {
    gameSession.diaSupport = deviceinfo_diaSupported();
    gameSession.colorSupport = deviceinfo_colorSupported();

    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.lastPenInput = (InputPen){0};

    gameSession.pawns = NULL;
    gameSession.activePawn = NULL;

    gameSession.pawns = MemPtrNew(sizeof(Pawn) * 3);
    MemSet(gameSession.pawns, sizeof(Pawn) * 3, 0);
    gameSession.pawnCount = 3;
    gameSession.pawns[0] = (Pawn){(Coordinate){2, 3}, 0, false, 0};
    gameSession.pawns[1] = (Pawn){(Coordinate){5, 4}, 0, false, 0};
    gameSession.pawns[2] = (Pawn){(Coordinate){1, 4}, 0, false, 1};
    gameSession.activePawn = &gameSession.pawns[0];

    gameSession.drawingState = (DrawingState){true, true, (Coordinate){0, 0}};
    gameSession.highlightTiles = NULL;
    gameSession.highlightTileCount = 0;
    gameSession.secondaryHighlightTiles = NULL;
    gameSession.secondaryHighlightTileCount = 0;

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

static UInt8 gameSession_maxRange(TargetSelectionType targetSelectionType) {
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
            //maxTileRange = ;
            coordinatesCount = gameSession.pawnCount;
            coordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * coordinatesCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                coordinates[i] = gameSession.pawns[i].position;
            }
            movement_findTilesInRange(currentPosition, maxTileRange, coordinates, coordinatesCount, &gameSession.highlightTiles, &gameSession.highlightTileCount);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            gameSession.highlightTiles = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.pawnCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (gameSession.pawns[i].faction == gameSession.activePawn->faction) {
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
    Pawn *selectedPawn;
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
            gameSession.movement->launchTimestamp = TimGetTicks();
            gameSession.movement->trajectory = movement_trajectoryBetween((Coordinate){gameSession.activePawn->position.x, gameSession.activePawn->position.y}, selectedTile);
            gameSession.movement->pawn = gameSession.activePawn;
            gameSession.activePawn->position = selectedTile;
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            // TODO: Trigger attack
            selectedPawn = gameSession_pawnAtTile(selectedTile);
            gameSession.state = GAMESTATE_DEFAULT;
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
}

AppColor gameSession_factionColor(UInt8 faction) {
    switch (faction % 5) {
        case 0:
            return EMERALD;
        case 1:
            return CLOUDS;
        case 2:
            return DIRT;
        case 3:
            return BELIZEHOLE;
        default:
            return DRACULAORCHID;  // Default case, should not be reached
    }
}