#include "gamesession.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "hexgrid.h"
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
    int i;
    const char *buttonTexts[] = {cancelText, gameSession.activePawn->cloaked ? decloakText : cloakText, torpedoText, phaserText, moveText};
    gameSession.displayButtonCount = 5;
    gameSession.displayButtons = (Button *)MemPtrNew(sizeof(Button) * 5);

    for (i = 0; i < gameSession.displayButtonCount; i++) {
        gameSession.displayButtons[i].text = (char *)MemPtrNew(StrLen(buttonTexts[i]) + 1);
        StrCopy(gameSession.displayButtons[i].text, buttonTexts[i]);
        gameSession.displayButtons[i].length = StrLen(buttonTexts[i]);
    }

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

    gameSession.shouldRedrawOverlay = true;
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

static void gameSession_progressUpdateMovement() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    if (gameSession.movement == NULL) {
        return;
    }
    gameSession.shouldRedrawOverlay = true;

    timeSinceLaunch = TimGetTicks() - gameSession.movement->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * ((float)gameSession.movement->trajectory.tileCount - 1) / 1.7);
    gameSession.movement->pawnPosition = movement_coordinateAtPercentageOfTrajectory(gameSession.movement->trajectory, timePassedScale);

    drawhelper_drawTextWithValue("time: ", timePassedScale * 100, (Coordinate){0, 0});
    if (timePassedScale >= 1) {
        gameSession_clearMovement();
        gameSession.state = GAMESTATE_DEFAULT;
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
    }
    gameSession_progressUpdateMovement();
}
