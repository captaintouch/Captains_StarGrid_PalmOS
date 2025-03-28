#include "gamesession.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gameActionLogic.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "movement.h"
#include "pawnActionMenuViewModel.h"
#include "viewport.h"

#define GAME_LOGIC_TICK 20

void gameSession_initialize() {
    gameSession.diaSupport = deviceinfo_diaSupported();
    gameSession.colorSupport = deviceinfo_colorSupported();
    gameSession.timeBetweenLogicProgressions = SysTicksPerSecond() / GAME_LOGIC_TICK;

    gameSession.state = GAMESTATE_DEFAULT;
    gameSession.lastPenInput = (InputPen){false, false, (Coordinate){-1, -1}};

    gameSession.pawns = NULL;
    gameSession.activePawn = NULL;

    gameSession.drawingState = (DrawingState){true, true, false, false, (Coordinate){0, 0}, (Coordinate){0, 0}};

    if (gameSession.pawns != NULL) {
        gameSession.pawnCount = 0;
        MemPtrFree(gameSession.pawns);
    }
    gameSession.pawns = MemPtrNew(sizeof(Pawn) * 8);
    MemSet(gameSession.pawns, sizeof(Pawn) * 8, 0);
    gameSession.pawnCount = 8;
    gameSession.pawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 0, false, false};
    gameSession.pawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 0, false, false};
    gameSession.pawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 8}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 1, true, false};
    gameSession.pawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){8, 7}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 1, false, false};
    gameSession.pawns[4] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 2, false, false};

    gameSession.pawns[5] = (Pawn){PAWNTYPE_BASE, (Coordinate){1, 1}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 0, 0, true}, 0, 0, false, false};
    gameSession.pawns[6] = (Pawn){PAWNTYPE_BASE, (Coordinate){8, 8}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 1, 0, true}, 0, 1, false, false};
    gameSession.pawns[7] = (Pawn){PAWNTYPE_BASE, (Coordinate){1, 7}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 2, 0, true}, 0, 2, false, false};

    gameSession.factionTurn = 0;
    gameSession.playerFaction = 0;
    gameSession.drawingState.shouldDrawButtons = gameSession.factionTurn == gameSession.playerFaction;

    gameSession.activePawn = &gameSession.pawns[0];

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

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition, TargetSelectionType targetSelectionType) {
    int i;
    int maxTileRange = gameActionLogic_maxRange(targetSelectionType);
    Coordinate *coordinates = NULL;
    int coordinatesCount = 0;
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            coordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.pawnCount);
            for (i = 0; i < gameSession.pawnCount; i++) {
                Boolean isCloakedShipFromOtherFaction = gameSession.pawns[i].type == PAWNTYPE_SHIP && gameSession.pawns[i].faction != gameSession.activePawn->faction && gameSession.pawns[i].cloaked;
                if (!isCloakedShipFromOtherFaction && gameSession.pawns[i].type != PAWNTYPE_BASE) {
                    coordinates[coordinatesCount] = gameSession.pawns[i].position;
                    coordinatesCount++;
                }
            }
            MemPtrResize(coordinates, sizeof(Coordinate) * coordinatesCount);
            movement_findTilesInRange(currentPosition, maxTileRange, coordinates, coordinatesCount, &gameSession.highlightTiles, &gameSession.highlightTileCount);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            movement_findTilesInRange(currentPosition, maxTileRange, NULL, 0, &gameSession.secondaryHighlightTiles, &gameSession.secondaryHighlightTileCount);
            gameSession.highlightTiles = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.secondaryHighlightTileCount);
            for (i = 0; i < gameSession.secondaryHighlightTileCount; i++) {
                Pawn *pawnAtPosition = gameSession_pawnAtTile(gameSession.secondaryHighlightTiles[i]);
                if (pawnAtPosition == NULL || pawnAtPosition->faction == gameSession.activePawn->faction) {
                    continue;
                }
                gameSession.highlightTiles[coordinatesCount] = pawnAtPosition->position;
                coordinatesCount++;
            }
            coordinatesCount++;
            gameSession.highlightTileCount = coordinatesCount;
            MemPtrResize(gameSession.highlightTiles, sizeof(Coordinate) * coordinatesCount);
            break;
    }

    if (coordinates != NULL) {
        MemPtrFree(coordinates);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_showPawnActions() {
    if (gameSession.activePawn->turnComplete) {
        FrmCustomAlert(GAME_ALERT_NOMOREACTIONS, NULL, NULL, NULL);
        return;
    }
    pawnActionMenuViewModel_setupMenuForPawn(gameSession.activePawn, &gameSession.displayButtons, &gameSession.displayButtonCount);
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.state = GAMESTATE_CHOOSEPAWNACTION;
}

static int gameSession_pawnCount(Coordinate location) {
    int i;
    int count = 0;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (isEqualCoordinate(gameSession.pawns[i].position, location)) {
            count++;
        }
    }
    return count;
}

static void gameSession_enableActionsForFaction(int faction) {
    int i;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].faction == faction && gameSession.pawns[i].type == PAWNTYPE_SHIP) {
            gameSession.pawns[i].turnComplete = (gameSession.pawns[i].cloaked && gameSession_pawnCount(gameSession.pawns[i].position) > 1);
            gameSession.activePawn = &gameSession.pawns[i];
        }
    }
}

static Boolean gameSession_movesLeftForFaction(int faction) {
    int i;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].faction == faction && gameSession.pawns[i].type == PAWNTYPE_SHIP && !gameSession.pawns[i].turnComplete && !isInvalidCoordinate(gameSession.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

static int gameSession_nextAvailableFaction(int currentFaction) {
    int i;
    int nextFaction;
    int factionCount = 0;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].faction > factionCount) {
            factionCount = gameSession.pawns[i].faction;
        }
    }
    factionCount++;

    nextFaction = (currentFaction + 1) % factionCount;
    while (!gameSession_movesLeftForFaction(nextFaction)) {
        nextFaction = (nextFaction + 1) % factionCount;
    }
    return nextFaction;
}

static Pawn *gameSession_nextPawn() {
    Pawn *firstPawn = NULL;
    Pawn *currentPawn = gameSession.activePawn->faction == gameSession.factionTurn ? gameSession.activePawn : NULL;
    int i;
    int startMatching = false;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].faction == gameSession.factionTurn && gameSession.pawns[i].type == PAWNTYPE_SHIP && !isInvalidCoordinate(gameSession.pawns[i].position)) {
            if (startMatching) {
                return &gameSession.pawns[i];
            }
            if (firstPawn == NULL) {
                firstPawn = &gameSession.pawns[i];
            }
            if (currentPawn == &gameSession.pawns[i]) {
                startMatching = true;
            }
        }
    }
    return firstPawn;
}

static void gameSession_startTurnForNextFaction() {
    gameSession_enableActionsForFaction(gameSession.factionTurn);
    gameSession.factionTurn = gameSession_nextAvailableFaction(gameSession.factionTurn);
    gameSession.drawingState.shouldDrawButtons = gameSession.factionTurn == gameSession.playerFaction;
    gameSession.activePawn = gameSession_nextPawn();
    gameSession_updateViewPortOffset(true);
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static Boolean gameSession_handleBarButtonsTap() {
    if (gameSession.lastPenInput.touchCoordinate.x > gameSession.drawingState.barButtonPositions[0].x && gameSession.lastPenInput.touchCoordinate.y > gameSession.drawingState.barButtonPositions[0].y && gameSession.lastPenInput.touchCoordinate.y < gameSession.drawingState.barButtonPositions[0].y + gameSession.drawingState.barButtonHeight) {  // Next button
        gameSession.activePawn = gameSession_nextPawn();
        gameSession_updateViewPortOffset(true);
        gameSession.drawingState.shouldRedrawOverlay = true;
        return true;
    } else if (gameSession.lastPenInput.touchCoordinate.x > gameSession.drawingState.barButtonPositions[1].x && gameSession.lastPenInput.touchCoordinate.y > gameSession.drawingState.barButtonPositions[1].y && gameSession.lastPenInput.touchCoordinate.y < gameSession.drawingState.barButtonPositions[1].y + gameSession.drawingState.barButtonHeight) {  // end turn button
        gameSession_startTurnForNextFaction();
        return true;
    }
    return false;
}

static Boolean gameSession_handleTileTap() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (selectedPawn != NULL && selectedPawn->faction == gameSession.playerFaction) {
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
    gameSession.drawingState.shouldRedrawOverlay = true;
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
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (!gameSession_highlightTilesContains(selectedTile)) {
        gameSession_resetHighlightTiles();
        gameSession.state = GAMESTATE_DEFAULT;
        gameSession.drawingState.shouldRedrawOverlay = true;
        return;
    }

    gameSession.activePawn->turnComplete = true;
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            gameActionLogic_scheduleMovement(selectedPawn, selectedTile);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            gameActionLogic_scheduleAttack(selectedPawn, selectedTile, gameSession.targetSelectionType);
            break;
    }

    gameSession_resetHighlightTiles();
    gameSession.drawingState.shouldRedrawOverlay = true;
}

Boolean gameSession_shouldShowHealthBar() {
    return gameSession.state == GAMESTATE_SELECTTARGET &&
           (gameSession.targetSelectionType == TARGETSELECTIONTYPE_PHASER || gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO) &&
           gameSession.movement == NULL;
}

void gameSession_scheduleNextGameLogicProgression() {
    gameSession.nextGameLogicProgressionTime = TimGetTicks() + gameSession.timeBetweenLogicProgressions;
}

static void gameSession_handlePawnActionButtonSelection() {
    int i;
    int selectedIndex = bottomMenu_selectedIndex(gameSession.lastPenInput.touchCoordinate);

    if (selectedIndex >= gameSession.displayButtonCount) {
        return;
    }

    if (gameSession.displayButtons[selectedIndex].disabled) {
        return;
    }

    switch (pawnActionMenuViewModel_actionAtIndex(selectedIndex, gameSession.activePawn)) {
        case MenuActionTypeCancel:
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case MenuActionTypeCloak:
            gameSession.activePawn->turnComplete = true;
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

static void gameSession_progressUpdateExplosion() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->explosionTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * gameSession.attackAnimation->explosionDurationSeconds);
    if (timePassedScale >= 1) {
        gameActionLogic_clearAttack();
        gameSession.state = GAMESTATE_DEFAULT;
    }
}

static void gameSession_progressUpdateAttack() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    Coordinate targetCenter, sourceCenter;
    Line attackLine;
    if (gameSession.attackAnimation == NULL) {
        return;
    }
    if (!isInvalidCoordinate(gameSession.attackAnimation->explosionPosition)) {
        gameSession_progressUpdateExplosion();
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * gameSession.attackAnimation->durationSeconds);

    if (gameSession.attackAnimation->lines != NULL) {
        MemPtrFree(gameSession.attackAnimation->lines);
    }

    targetCenter = hexgrid_tileCenterPosition(gameSession.attackAnimation->target);
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            break;
        case TARGETSELECTIONTYPE_PHASER:
            attackLine = (Line){hexgrid_tileCenterPosition(gameSession.activePawn->position), gameSession_getBoxCoordinate(targetCenter, timePassedScale, HEXTILE_PAWNSIZE / 3)};
            gameSession.attackAnimation->lines = (Line *)MemPtrNew(sizeof(Line) * 3);
            gameSession.attackAnimation->lines[0] = (Line){attackLine.startpoint, movement_coordinateAtPercentageOfLine(attackLine, remapToMax(timePassedScale * 2.4, 1))};
            attackLine.startpoint = gameSession.attackAnimation->lines[0].endpoint;
            gameSession.attackAnimation->lines[1] = (Line){gameSession.attackAnimation->lines[0].endpoint, movement_coordinateAtPercentageOfLine(attackLine, 0.7)};
            gameSession.attackAnimation->lines[2] = (Line){gameSession.attackAnimation->lines[1].endpoint, attackLine.endpoint};
            gameSession.attackAnimation->lineCount = 3;
            gameSession.attackAnimation->torpedoPosition = (Coordinate){-1, -1};
            break;
        case TARGETSELECTIONTYPE_TORPEDO:
            sourceCenter = hexgrid_tileCenterPosition(gameSession.activePawn->position);
            gameSession.attackAnimation->torpedoPosition = movement_coordinateAtPercentageOfLine((Line){sourceCenter.x, sourceCenter.y, targetCenter.x, targetCenter.y}, timePassedScale);
            break;
    }

    if (timePassedScale >= 1) {
        gameActionLogic_afterAttack();
        if (gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO || isInvalidCoordinate(gameSession.attackAnimation->targetPawn->position)) {  // show explosion when destroyed or always when torpedo is used
            gameSession.attackAnimation->explosionPosition = targetCenter;
            gameSession.attackAnimation->explosionTimestamp = TimGetTicks();
            gameSession.attackAnimation->explosionDurationSeconds = 0.5;
        } else {
            gameActionLogic_clearAttack();
            gameSession.state = GAMESTATE_DEFAULT;
        }
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
        if (!gameActionLogic_afterMove()) {
            gameSession_updateViewPortOffset(true);
        }
    }
}

static void gameSession_cpuTurn() {
    int i;
    if (gameSession.attackAnimation != NULL || gameSession.movement != NULL || gameSession.state != GAMESTATE_DEFAULT) {
        return;
    }
    for (i = 0; i < gameSession.pawnCount; i++) {
        Coordinate closestTile;
        Pawn *targetPawn;
        CPUStrategyResult strategy;
        Pawn *pawn = &gameSession.pawns[i];
        if (pawn->faction != gameSession.factionTurn || pawn->type != PAWNTYPE_SHIP || pawn->turnComplete) {
            continue;
        }
        strategy = cpuLogic_getStrategy(pawn, gameSession.pawns, gameSession.pawnCount);
        pawn->turnComplete = true;
        gameSession.activePawn = pawn;
        gameSession_updateViewPortOffset(true);
        switch (strategy.CPUAction) {
            case CPUACTION_MOVE:
                closestTile = movement_closestTileToTargetInRange(pawn, strategy.target, gameSession.pawns, gameSession.pawnCount, true);
                if (isEqualCoordinate(closestTile, strategy.target->position)) {
                    targetPawn = strategy.target;
                } else {
                    targetPawn = NULL;
                }
                gameActionLogic_scheduleMovement(targetPawn, closestTile);
                StrCopy(gameSession.cpuActionText, "Moving");
                break;
            case CPUACTION_PHASERATTACK:
            case CPUACTION_TORPEDOATTACK:
                gameActionLogic_scheduleAttack(strategy.target, strategy.target->position, strategy.CPUAction == CPUACTION_PHASERATTACK ? TARGETSELECTIONTYPE_PHASER : TARGETSELECTIONTYPE_TORPEDO);
                StrCopy(gameSession.cpuActionText, "Attacking");
                break;
            case CPUACTION_CLOAK:
                pawn->cloaked = !pawn->cloaked;
                if (pawn->cloaked) {
                    StrCopy(gameSession.cpuActionText, "Cloaking");
                } else {
                    StrCopy(gameSession.cpuActionText, "Decloaking");
                }
                gameSession.drawingState.requiresPauseAfterLayout = true;
                break;
            case CPUACTION_NONE:
                StrCopy(gameSession.cpuActionText, "No action");
                gameSession.drawingState.requiresPauseAfterLayout = true;
                break;
        }
        return;
    }
    if (!gameSession_movesLeftForFaction(gameSession.factionTurn)) {
        gameSession_startTurnForNextFaction();
    }
}

void gameSession_progressLogic() {
    if (gameSession.playerFaction != gameSession.factionTurn) {
        gameSession_cpuTurn();
    } else if (gameSession.lastPenInput.wasUpdatedFlag) {  // handle user actions
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
                    if (gameSession_handleBarButtonsTap()) break;
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
