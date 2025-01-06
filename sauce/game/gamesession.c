#include "gamesession.h"

#include "../constants.h"
#include "hexgrid.h"

void gameSession_initialize() {
    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.lastPenInput = (InputPen){0};

    gameSession.pawns = NULL;
    gameSession.activePawn = NULL;

    gameSession.pawns = MemPtrNew(sizeof(Pawn) * 1);
    gameSession.pawnCount = 1;
    gameSession.pawns[0] = (Pawn){(Coordinate){2, 3}};
    gameSession.activePawn = &gameSession.pawns[0];

    gameSession.shouldRedrawOverlay = false;
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;

    gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;
}

void gameSession_registerPenInput(EventPtr eventptr) {
    // Input we get is for the entire screen, we need to offset it so that it matches our playing field area
    inputPen_updateEventDetails(&gameSession.lastPenInput, eventptr, -GAMEWINDOW_X, -GAMEWINDOW_Y);
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
    int i, j;
    Coordinate *positions;
    int positionCount = 0;
    int maxTileRange = 0;
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            maxTileRange = GAMEMECHANICS_MAXTILEMOVERANGE;
            break;
        case TARGETSELECTIONTYPE_PHASER:
            maxTileRange = GAMEMECHANICS_MAXTILEPHASERRANGE;
            break;
        case TARGETSELECTIONTYPE_TORPEDO:
            maxTileRange = GAMEMECHANICS_MAXTILETORPEDORANGE;
            break;
    }
    positions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * maxTileRange * 2 * maxTileRange * 2);
    for (i = -maxTileRange + 1; i < maxTileRange; i++) {
        for (j = -maxTileRange + 1; j < maxTileRange; j++) {
            Coordinate newPosition = (Coordinate){currentPosition.x + i, currentPosition.y + j};
            if (newPosition.x == currentPosition.x && newPosition.y == currentPosition.y) {
                continue;
            }
            if (newPosition.x >= 0 && newPosition.x < HEXGRID_COLS && newPosition.y >= 0 && newPosition.y < HEXGRID_ROWS) {
                positions[positionCount] = newPosition;
                positionCount++;
            }
        }
    }
    MemPtrResize(positions, positionCount * sizeof(Coordinate));
    gameSession.specialTiles = positions;
    gameSession.specialTileCount = positionCount;
    gameSession.shouldRedrawOverlay = true;
}

static void gameSession_handleTileTap() {
    Coordinate selectedTile = hexgrid_tileAtPixel(gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (selectedPawn != NULL) {
        gameSession.state = GAMESTATE_SELECTTARGET;
        gameSession.activePawn = selectedPawn;
        gameSession_updateValidPawnPositionsForMovement(selectedPawn->position, gameSession.targetSelectionType);
    }
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

static void gameSession_handleTargetSelection() {
    Coordinate selectedTile = hexgrid_tileAtPixel(gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y);
    if (!gameSession_specialTilesContains(selectedTile)) {
        return;
    }
    
    gameSession.activePawn->position = selectedTile;
    MemPtrFree(gameSession.specialTiles);
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;
    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.shouldRedrawOverlay = true;
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

void gameSession_progressLogic() {
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        // Handle pen input
        gameSession.lastPenInput.wasUpdatedFlag = false;

        switch (gameSession.state) {
            case GAMESTATE_DEFAULT:
                gameSession_handleTileTap();
                break;
            case GAMESTATE_SELECTTARGET:
                gameSession_handleTargetSelection();
                break;
        }
        // if the user selected a SHIP, show the possible moves
    }
}
