#include "gamesession.h"

#include "../constants.h"
#include "hexgrid.h"

void gameSession_initialize() {
    gameSession.lastPenInput = (InputPen){0};

    gameSession.pawns = NULL;
    gameSession.pawns = MemPtrNew(sizeof(Pawn) * 1);
    gameSession.pawnCount = 1;
    gameSession.pawns[0] = (Pawn){(Coordinate){2, 3}};
    // gameSession.activePawn = &gameSession.pawns[0];

    gameSession.shouldRedrawOverlay = false;
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;
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

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition) {
    int i, j;
    Coordinate *positions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * GAMEMECHANICS_MAXTILEMOVES * 2 * GAMEMECHANICS_MAXTILEMOVES * 2);
    int positionCount = 0;
    for (i = -GAMEMECHANICS_MAXTILEMOVES; i < GAMEMECHANICS_MAXTILEMOVES; i++) {
        for (j = -GAMEMECHANICS_MAXTILEMOVES; j < GAMEMECHANICS_MAXTILEMOVES; j++) {
            Coordinate newPosition = (Coordinate){currentPosition.x + i, currentPosition.y + j};
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

void gameSession_progressLogic() {
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        Coordinate selectedTile;
        Pawn *selectedPawn;

        // Handle pen input
        gameSession.lastPenInput.wasUpdatedFlag = false;

        // if the user selected a SHIP, show the possible moves
        selectedTile = hexgrid_tileAtPixel(gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y);
        selectedPawn = gameSession_pawnAtTile(selectedTile);
        if (selectedPawn != NULL) {
            gameSession_updateValidPawnPositionsForMovement(selectedPawn->position);
        }
    }
}
