#include "gamesession.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "movement.h"
#include "viewport.h"

#define moveText "Move"
#define phaserText "Phaser"
#define torpedoText "Torpedo"
#define cloakText "Cloak"
#define decloakText "Decloak"
#define cancelText "Cancel"

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
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;

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

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition, TargetSelectionType targetSelectionType) {
    int maxTileRange = 0;
    int i;
    Coordinate *invalidCoordinates = NULL;
    int invalidCoordinatesCount = 0;
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            maxTileRange = GAMEMECHANICS_MAXTILEMOVERANGE;
            invalidCoordinatesCount = gameSession.pawnCount;
            invalidCoordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * invalidCoordinatesCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                invalidCoordinates[i] = gameSession.pawns[i].position;
            }
            break;
        case TARGETSELECTIONTYPE_PHASER:
            maxTileRange = GAMEMECHANICS_MAXTILEPHASERRANGE;
            break;
        case TARGETSELECTIONTYPE_TORPEDO:
            maxTileRange = GAMEMECHANICS_MAXTILETORPEDORANGE;
            break;
    }

    movement_updateValidPawnPositionsForMovement(currentPosition, maxTileRange, invalidCoordinates, invalidCoordinatesCount, &gameSession.specialTiles, &gameSession.specialTileCount);
    if (invalidCoordinates != NULL) {
        MemPtrFree(invalidCoordinates);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_showPawnActions() {
    int i;
    const char *buttonTexts[] = {cancelText, gameSession.activePawn->cloaked ? decloakText : cloakText, torpedoText, phaserText, moveText};
    gameSession.displayButtonCount = 5;
    gameSession.displayButtons = (Button *)MemPtrNew(sizeof(Button) * 5);

    for (i = 0; i < gameSession.displayButtonCount; i++) {
        gameSession.displayButtons[i].text = (char *)MemPtrNew(StrLen(buttonTexts[i]) + 1);
        StrCopy(gameSession.displayButtons[i].text, buttonTexts[i]);
        gameSession.displayButtons[i].length = StrLen(buttonTexts[i]);
    }

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

static Boolean gameSession_specialTilesContains(Coordinate coordinate) {
    int i;
    for (i = 0; i < gameSession.specialTileCount; i++) {
        if (gameSession.specialTiles[i].x == coordinate.x && gameSession.specialTiles[i].y == coordinate.y) {
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

static void gameSession_handleTargetSelection() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    if (!gameSession_specialTilesContains(selectedTile)) {
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
            break;
        case TARGETSELECTIONTYPE_TORPEDO:
            break;
    }

    MemPtrFree(gameSession.specialTiles);
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;

    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_handlePawnActionButtonSelection() {
    int i;
    int selectedIndex = bottomMenu_selectedIndex(gameSession.lastPenInput.touchCoordinate);

    if (selectedIndex >= gameSession.displayButtonCount) {
        return;
    }

    switch (selectedIndex) {
        case 0:  // CANCEL
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case 1:  // CLOAK-DECLOAK
            gameSession.state = GAMESTATE_DEFAULT;
            gameSession.activePawn->cloaked = !gameSession.activePawn->cloaked;
            break;
        case 2:  // TORPEDO
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_TORPEDO;
            break;
        case 3:  // PHASER
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_PHASER;
            break;
        case 4:  // MOVE
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

AppColor gameSession_specialTilesColor() {
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