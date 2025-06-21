#include "gameActionLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "models.h"
#include "movement.h"

static UInt8 gameActionLogic_nonCapturedFlagsLeft(UInt8 faction) {
    int i;
    UInt8 flagsLeft = 0;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        // Count of enemies that are currently carrying a flag (including bases)
        // + Count of own ships that are currently carrying a flag (excluding home base)
        if (isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            continue;
        }
        if ((gameSession.level.pawns[i].faction != faction && gameSession.level.pawns[i].inventory.carryingFlag) ||
            (gameSession.level.pawns[i].faction == faction && gameSession.level.pawns[i].inventory.carryingFlag && gameSession.level.pawns[i].type != PAWNTYPE_BASE)) {
            flagsLeft++;
        }
    }
    return flagsLeft;
}

/*static void gameActionLogic_restartGame() {
    FrmCustomAlert(GAME_ALERT_ENDOFGAMETECHDEMO, NULL, NULL, NULL);
    gameSession_reset(false);
}*/

static void gameActionLogic_showScore() {
    Coordinate oldPosition = gameSession.activePawn->position;
    gameSession.menuScreenType = MENUSCREEN_SCORE;
    gameSession.drawingState.shouldRedrawBackground = true;
    gameSession.drawingState.shouldRedrawHeader = true;
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.activePawn->type = PAWNTYPE_SHIP;
    gameSession.level.scores[gameSession.activePawn->faction].shipsDestroyed[1] = 2;
    gameSession.level.scores[gameSession.activePawn->faction].shipsDestroyed[2] = 1;
    gameSession.level.scores[gameSession.activePawn->faction].shipsDestroyed[3] = 2;
    gameSession.level.scores[gameSession.activePawn->faction].shipsCaptured[3] = 2;
    gameSession.level.scores[gameSession.activePawn->faction].basesDestroyed[2] = true;
    gameSession.level.scores[gameSession.activePawn->faction].basesDestroyed[3] = true;
    gameSession.level.scores[gameSession.activePawn->faction].flagsStolen = 5;
    gameSession.level.scores[3].shipsDestroyed[gameSession.activePawn->faction] = 2;
    gameSession.level.scores[2].shipsDestroyed[gameSession.activePawn->faction] = 3;
    scoring_saveScore(gameSession.level.scores, gameSession.activePawn->faction);
    level_addScorePawns(&gameSession.level, gameSession.activePawn->faction);
    gameSession.activePawn = gameSession_pawnAtTile(oldPosition);
    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETLEFT, 0});
}

static Boolean gameActionLogic_humanShipsLeft() {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.factions[gameSession.level.pawns[i].faction].human && gameSession.level.pawns[i].type == PAWNTYPE_SHIP && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

static Boolean gameActionLogic_enemyShipsLeft() {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].type == PAWNTYPE_SHIP && gameSession.level.pawns[i].faction != gameSession.activePawn->faction && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

void gameActionLogic_clearMovement() {
    if (gameSession.movement != NULL) {
        if (gameSession.movement->trajectory.tileCoordinates != NULL) {
            MemPtrFree(gameSession.movement->trajectory.tileCoordinates);
            gameSession.movement->trajectory.tileCoordinates = NULL;
        }
        MemPtrFree(gameSession.movement);
        gameSession.movement = NULL;
    }
}

void gameActionLogic_scheduleWarp(Pawn *sourcePawn, Coordinate target) {
    gameSession.warpAnimation.pawn = sourcePawn;
    gameSession.warpAnimation.isWarping = true;
    gameSession.warpAnimation.currentPosition = sourcePawn->position;
    gameSession.warpAnimation.endPosition = target;
    gameSession.warpAnimation.launchTimestamp = TimGetTicks();
    gameSession.warpAnimation.shipVisible = true;
}

void gameActionLogic_scheduleShockwave(Pawn *basePawn) {
    int i, affectedPawnCount = 0;
    gameSession.shockWaveAnimation = (ShockWaveAnimation *)MemPtrNew(sizeof(ShockWaveAnimation));
    MemSet(gameSession.shockWaveAnimation, sizeof(ShockWaveAnimation), 0);
    gameSession.shockWaveAnimation->launchTimestamp = TimGetTicks();
    gameSession.shockWaveAnimation->basePawn = basePawn;
    gameSession.shockWaveAnimation->affectedPawnIndices = (int *)MemPtrNew(sizeof(int) * gameSession.level.pawnCount);
    gameSession.shockWaveAnimation->pawnOriginalPositions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.level.pawnCount);

    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (!isInvalidCoordinate(gameSession.level.pawns[i].position) && gameSession.level.pawns[i].type == PAWNTYPE_SHIP && movement_distance(basePawn->position, gameSession.level.pawns[i].position) < GAMEMECHANICS_SHOCKWAVERANGE - 1) {
            gameSession.shockWaveAnimation->affectedPawnIndices[affectedPawnCount] = i;
            gameSession.shockWaveAnimation->pawnOriginalPositions[affectedPawnCount] = gameSession.level.pawns[i].position;
            gameSession.level.pawns[i].position = movement_positionAwayFrom(basePawn->position, &gameSession.level.pawns[i], gameSession.level.pawns, gameSession.level.pawnCount, GAMEMECHANICS_SHOCKWAVERANGE);
            affectedPawnCount++;
        }
    }
    gameSession.shockWaveAnimation->affectedPawnCount = affectedPawnCount;
    MemPtrResize(gameSession.shockWaveAnimation->affectedPawnIndices, sizeof(int) * affectedPawnCount);
}

void gameActionLogic_scheduleMovement(Pawn *sourcePawn, Pawn *targetPawn, Coordinate selectedTile) {
    gameActionLogic_clearMovement();

    gameSession.movement = (Movement *)MemPtrNew(sizeof(Movement));
    MemSet(gameSession.movement, sizeof(Movement), 0);
    gameSession.movement->launchTimestamp = TimGetTicks();
    gameSession.movement->targetPawn = targetPawn;

    if (sourcePawn == &gameSession.cameraPawn) {
        gameSession.movement->trajectory.tileCoordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * 2);
        gameSession.movement->trajectory.tileCount = 2;
        gameSession.movement->trajectory.tileCoordinates[0] = (Coordinate){gameSession.cameraPawn.position.x, gameSession.cameraPawn.position.y};
        gameSession.movement->trajectory.tileCoordinates[1] = selectedTile;
    } else {
        gameSession.movement->trajectory = movement_trajectoryBetween((Coordinate){sourcePawn->position.x, sourcePawn->position.y}, selectedTile);
    }
    gameSession.movement->pawnPosition = hexgrid_tileCenterPosition(sourcePawn->position);
    gameSession.movement->pawn = sourcePawn;
    sourcePawn->position = selectedTile;
}

static void gameActionLogic_returnFlagToBase(Pawn *pawn) {
    int i;
    if (!pawn->inventory.carryingFlag) {
        return;
    }
    pawn->inventory.carryingFlag = false;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].faction == pawn->inventory.flagOfFaction && gameSession.level.pawns[i].type == PAWNTYPE_BASE) {
            gameSession.level.pawns[i].inventory.carryingFlag = true;
            return;
        }
    }
}

static Boolean gameActionLogic_baseOnPosition(Coordinate position) {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (!isInvalidCoordinate(gameSession.level.pawns[i].position) && isEqualCoordinate(gameSession.level.pawns[i].position, position) && gameSession.level.pawns[i].type == PAWNTYPE_BASE) {
            return true;
        }
    }
    return false;
}

static Boolean gameActionLogic_checkForGameOver() {
    if (!gameSession.continueCPUPlay && !gameActionLogic_humanShipsLeft()) {
        if (FrmCustomAlert(GAME_ALERT_CPUCONTINUEPLAYING, NULL, NULL, NULL) != 0) {  // Do not continue playing
            gameActionLogic_showScore();
            return true;
        } else {
            gameSession.continueCPUPlay = true;
        }
    }
    return false;
}

// returns true when another movement has been scheduled
Boolean gameActionLogic_afterMove() {
    Boolean didScheduleMovement = false;
    Pawn *selectedPawn = gameSession.movement->targetPawn;
    if (gameSession.menuScreenType == MENUSCREEN_GAME) {
        gameActionLogic_showScore();
        return true;
    }

    StrCopy(gameSession.cpuActionText, "");
    // Check if flag was captured
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->inventory.carryingFlag && !gameSession.activePawn->inventory.carryingFlag && selectedPawn->inventory.flagOfFaction != gameSession.activePawn->faction) {
        gameSession.activePawn->inventory.carryingFlag = true;
        gameSession.activePawn->inventory.flagOfFaction = selectedPawn->inventory.flagOfFaction;
        selectedPawn->inventory.carryingFlag = false;
        gameSession.level.scores[gameSession.activePawn->faction].flagsStolen++;
    }
    // Check if flag was returned to player's base
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->faction == gameSession.activePawn->faction && gameSession.activePawn->inventory.carryingFlag) {
        // Flag dissapears, enemy base dissapears, enemy ships join the players fleet
        int i;
        for (i = 0; i < gameSession.level.pawnCount; i++) {
            if (isInvalidCoordinate(gameSession.level.pawns[i].position)) {
                continue;
            }
            if (gameSession.level.pawns[i].faction == gameSession.activePawn->inventory.flagOfFaction) {
                gameSession.level.scores[gameSession.activePawn->faction].shipsCaptured[gameSession.level.pawns[i].faction]++;
                gameSession.level.pawns[i].faction = gameSession.activePawn->faction;

                gameActionLogic_returnFlagToBase(&gameSession.level.pawns[i]);
                if (gameSession.level.pawns[i].type == PAWNTYPE_BASE) {
                    gameSession.level.pawns[i].position = (Coordinate){-1, -1};
                }
            }
        }
        gameSession.activePawn->inventory.carryingFlag = false;
        gameSession.level.scores[gameSession.activePawn->faction].flagsCaptured[gameSession.activePawn->inventory.flagOfFaction]++;
        if (gameActionLogic_nonCapturedFlagsLeft(gameSession.activePawn->faction) > 0) {  // Still some flags left to capture
            FrmCustomAlert(GAME_ALERT_FLAGCAPTURED, NULL, NULL, NULL);
            if (gameActionLogic_checkForGameOver()) {
                return false;
            }
        } else {  // Game over, all flags captured
            if (gameSession.factions[gameSession.activePawn->faction].human) {
                FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_ALLFLAGSCAPTURED, NULL, NULL, NULL);
            } else {
                FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_CPUALLFLAGSCAPTURED, NULL, NULL, NULL);
            }
            gameActionLogic_showScore();
        }
    }

    // if currentposition is on a base, move away from it
    if (gameActionLogic_baseOnPosition(gameSession.activePawn->position)) {
        Coordinate finalCoordinate = movement_closestTileToTargetInRange(gameSession.activePawn, gameSession.activePawn->position, gameSession.level.pawns, gameSession.level.pawnCount, false);
        gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, finalCoordinate);
        didScheduleMovement = true;
    } else {
        gameActionLogic_clearMovement();
        gameSession.state = GAMESTATE_DEFAULT;
    }
    return didScheduleMovement;
}

void gameActionLogic_afterAttack() {
    StrCopy(gameSession.cpuActionText, "");
    // Update inventory stats
    gameSession.attackAnimation->targetPawn->inventory.health -= gameSession.attackAnimation->healthImpact;
    if (gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO) {
        gameSession.activePawn->inventory.torpedoCount--;
    }

    if (gameSession.attackAnimation->targetPawn->inventory.health <= 0) {
        gameSession.attackAnimation->targetPawn->inventory.health = 0;
        gameSession.attackAnimation->targetPawn->position = (Coordinate){-1, -1};
        gameSession.level.scores[gameSession.activePawn->faction].shipsDestroyed[gameSession.attackAnimation->targetPawn->faction]++;

        if (gameSession.attackAnimation->targetPawn->type == PAWNTYPE_BASE) {
            int i;
            for (i = 0; i < gameSession.level.pawnCount; i++) {
                if (isInvalidCoordinate(gameSession.level.pawns[i].position)) {
                    continue;
                }
                if (gameSession.level.pawns[i].faction == gameSession.attackAnimation->targetPawn->faction) {
                    gameSession.level.pawns[i].faction = gameSession.activePawn->faction;
                }
            }
            FrmCustomAlert(GAME_ALERT_BASEDESTROYED, NULL, NULL, NULL);
        }
        gameActionLogic_returnFlagToBase(gameSession.attackAnimation->targetPawn);
    }

    if (gameActionLogic_checkForGameOver()) {
        return;
    }

    // Check for game over if no enemy units left
    if (!gameActionLogic_enemyShipsLeft()) {
        if (gameSession.factions[gameSession.activePawn->faction].human) {
            FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_TOTALDESTRUCTION, NULL, NULL, NULL);
        } else {
            FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_CPUTOTALDESTRUCTION, NULL, NULL, NULL);
        }
        gameActionLogic_showScore();
    }
}

UInt8 gameActionLogic_maxRange(TargetSelectionType targetSelectionType) {
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            return GAMEMECHANICS_MAXTILEMOVERANGE;
        case TARGETSELECTIONTYPE_PHASER:
            return GAMEMECHANICS_MAXTILEPHASERRANGE;
        case TARGETSELECTIONTYPE_TORPEDO:
            return GAMEMECHANICS_MAXTILETORPEDORANGE;
    }
}

static UInt8 gameActionLogic_healthImpact(Coordinate source, Coordinate target, TargetSelectionType attackType) {
    int maxRange;
    int distance;
    int maxImpact;
    switch (attackType) {
        case TARGETSELECTIONTYPE_MOVE:  // Shouldn't be triggered
        case TARGETSELECTIONTYPE_PHASER:
            maxRange = gameActionLogic_maxRange(attackType);
            distance = (maxRange - movement_distance(source, target) + 1);
            maxImpact = GAMEMECHANICS_MAXIMPACTPHASER;
            return (float)maxImpact * ((float)distance / (float)maxRange);
        case TARGETSELECTIONTYPE_TORPEDO:
            return GAMEMECHANICS_MAXIMPACTTORPEDO;
    }
}

static float gameActionLogic_attackDuration(Coordinate source, Coordinate target, TargetSelectionType attackType) {
    int distance;
    switch (attackType) {
        case TARGETSELECTIONTYPE_MOVE:  // Shouldn't be triggered
        case TARGETSELECTIONTYPE_PHASER:
            return 1.5;
        case TARGETSELECTIONTYPE_TORPEDO:
            distance = movement_distance(source, target) + 1;
            return (float)distance * 0.35;
    }
}

void gameActionLogic_clearShockwave() {
    if (gameSession.shockWaveAnimation != NULL) {
        if (gameSession.shockWaveAnimation->affectedPawnIndices != NULL) {
            MemPtrFree(gameSession.shockWaveAnimation->affectedPawnIndices);
            gameSession.shockWaveAnimation->affectedPawnIndices = NULL;
        }
        if (gameSession.shockWaveAnimation->pawnOriginalPositions != NULL) {
            MemPtrFree(gameSession.shockWaveAnimation->pawnOriginalPositions);
            gameSession.shockWaveAnimation->pawnOriginalPositions = NULL;
        }
        MemPtrFree(gameSession.shockWaveAnimation);
        gameSession.shockWaveAnimation = NULL;
    }
}

void gameActionLogic_clearAttack() {
    if (gameSession.attackAnimation != NULL) {
        if (gameSession.attackAnimation->lines != NULL) {
            MemPtrFree(gameSession.attackAnimation->lines);
            gameSession.attackAnimation->lines = NULL;
        }
        MemPtrFree(gameSession.attackAnimation);
        gameSession.attackAnimation = NULL;
    }
}

void gameActionLogic_scheduleAttack(Pawn *targetPawn, Coordinate selectedTile, TargetSelectionType attackType) {
    gameActionLogic_clearAttack();
    if (targetPawn != NULL) {
        gameSession.targetSelectionType = attackType;
        gameSession.attackAnimation = (AttackAnimation *)MemPtrNew(sizeof(AttackAnimation));
        MemSet(gameSession.attackAnimation, sizeof(AttackAnimation), 0);
        gameSession.attackAnimation->torpedoPosition = (Coordinate){-1, -1};
        gameSession.attackAnimation->explosionPosition = (Coordinate){-1, -1};
        gameSession.attackAnimation->launchTimestamp = TimGetTicks();
        gameSession.attackAnimation->target = selectedTile;
        gameSession.attackAnimation->targetPawn = targetPawn;
        gameSession.attackAnimation->healthImpact = gameActionLogic_healthImpact(gameSession.activePawn->position, selectedTile, gameSession.targetSelectionType);
        gameSession.attackAnimation->durationSeconds = gameActionLogic_attackDuration(gameSession.activePawn->position, selectedTile, gameSession.targetSelectionType);
        gameSession.activePawn->orientation = movement_orientationBetween(gameSession.activePawn->position, selectedTile);
    } else {
        gameSession.state = GAMESTATE_DEFAULT;
    }
}