#include "gamesession.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "hexgrid.h"
#include "viewport.h"

#define moveText "Move"
#define phaserText "Phaser"
#define torpedoText "Torpedo"
#define cloakText "Cloak"
#define cancelText "Cancel"

void gameSession_initialize() {
    gameSession.diaSupport = deviceinfo_diaSupported();

    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.lastPenInput = (InputPen){0};

    gameSession.pawns = NULL;
    gameSession.activePawn = NULL;

    gameSession.pawns = MemPtrNew(sizeof(Pawn) * 1);
    gameSession.pawnCount = 1;
    gameSession.pawns[0] = (Pawn){(Coordinate){2, 3}, false};
    gameSession.activePawn = &gameSession.pawns[0];

    gameSession.shouldRedrawOverlay = false;
    gameSession.specialTiles = NULL;
    gameSession.specialTileCount = 0;

    gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;

    gameSession.viewportOffset = (Coordinate){0, 0};
}

void gameSession_registerPenInput(EventPtr eventptr) {
    inputPen_updateEventDetails(&gameSession.lastPenInput, eventptr);
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

static void gameSession_showPawnActions() {
    gameSession.displayButtonCount = 5;
    gameSession.displayButtons = (Button *)MemPtrNew(sizeof(Button) * 5);

    gameSession.displayButtons[4].text = (char *)MemPtrNew(StrLen(moveText) + 1);
    StrCopy(gameSession.displayButtons[4].text, moveText);
    gameSession.displayButtons[4].length = StrLen(moveText);

    gameSession.displayButtons[3].text = (char *)MemPtrNew(StrLen(phaserText) + 1);
    StrCopy(gameSession.displayButtons[3].text, phaserText);
    gameSession.displayButtons[3].length = StrLen(phaserText);

    gameSession.displayButtons[2].text = (char *)MemPtrNew(StrLen(torpedoText) + 1);
    StrCopy(gameSession.displayButtons[2].text, torpedoText);
    gameSession.displayButtons[2].length = StrLen(torpedoText);

    gameSession.displayButtons[1].text = (char *)MemPtrNew(StrLen(cloakText) + 1);
    StrCopy(gameSession.displayButtons[1].text, cloakText);
    gameSession.displayButtons[1].length = StrLen(cloakText);

    gameSession.displayButtons[0].text = (char *)MemPtrNew(StrLen(cancelText) + 1);
    StrCopy(gameSession.displayButtons[0].text, cancelText);
    gameSession.displayButtons[0].length = StrLen(cancelText);

    gameSession.shouldRedrawOverlay = true;
    gameSession.state = GAMESTATE_CHOOSEPAWNACTION;
}

static void gameSession_handleTileTap() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (selectedPawn != NULL) {
        gameSession.activePawn = selectedPawn;
        gameSession_showPawnActions();
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
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    if (!gameSession_specialTilesContains(selectedTile)) {
        return;
    }

    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
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
    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.shouldRedrawOverlay = true;
}

static void gameSession_handlePawnActionButtonSelection() {
    int i;
    int selectedIndex = bottomMenu_selectedIndex(gameSession.lastPenInput.touchCoordinate);

    if (selectedIndex >= gameSession.displayButtonCount) {
        return;
    }

    switch (selectedIndex) {
        case 0:
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case 1:
            gameSession.state = GAMESTATE_DEFAULT;
            gameSession.activePawn->cloaked = true;
            break;
        case 2:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_TORPEDO;
            break;
        case 3:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_PHASER;
            break;
        case 4:
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
            case GAMESTATE_CHOOSEPAWNACTION:
                gameSession_handlePawnActionButtonSelection();
                break;
            case GAMESTATE_SELECTTARGET:
                gameSession_handleTargetSelection();
                break;
        }
        // if the user selected a SHIP, show the possible moves
    }
}
