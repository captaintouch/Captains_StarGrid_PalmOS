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
    for (i = -GAMEMECHANICS_MAXTILEMOVES + 1; i < GAMEMECHANICS_MAXTILEMOVES; i++) {
        for (j = -GAMEMECHANICS_MAXTILEMOVES + 1; j < GAMEMECHANICS_MAXTILEMOVES; j++) {
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
        gameSession.state = GAMESTATE_SELECTMOVE;
        gameSession.activePawn = selectedPawn;
        gameSession_updateValidPawnPositionsForMovement(selectedPawn->position);
    }
}

static void gameSession_handleMove() {
    Coordinate selectedTile = hexgrid_tileAtPixel(gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y);
    gameSession.activePawn->position = selectedTile;
    MemPtrFree(gameSession.specialTiles);
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;
    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.shouldRedrawOverlay = true;
}

void gameSession_progressLogic() {
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        // Handle pen input
        gameSession.lastPenInput.wasUpdatedFlag = false;

        switch (gameSession.state) {
            case GAMESTATE_DEFAULT:
                gameSession_handleTileTap();
                break;
            case GAMESTATE_SELECTMOVE:
                gameSession_handleMove();
                break;
        }
        // if the user selected a SHIP, show the possible moves
    }
}
