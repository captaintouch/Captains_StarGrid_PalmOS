#include "gameActionLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "models.h"
#include "movement.h"
#include "hexgrid.h"

static UInt8 gameActionLogic_nonCapturedFlagsLeft(UInt8 faction) {
    int i;
    UInt8 flagsLeft = 0;
    for (i = 0; i < gameSession.pawnCount; i++) {
        // Count of enemies that are currently carrying a flag (including bases)
        // + Count of own ships that are currently carrying a flag (excluding home base)
        if (isInvalidCoordinate(gameSession.pawns[i].position)) {
            continue;
        }
        if ((gameSession.pawns[i].faction != faction && gameSession.pawns[i].inventory.carryingFlag) ||
            (gameSession.pawns[i].faction == faction && gameSession.pawns[i].inventory.carryingFlag && gameSession.pawns[i].type != PAWNTYPE_BASE)) {
            flagsLeft++;
        }
    }
    return flagsLeft;
}

static UInt8 gameActionLogic_enemyUnitsLeft() {
    int i;
    UInt8 enemyUnits = 0;
    for (i = 0; i < gameSession.pawnCount; i++) {
        if (gameSession.pawns[i].faction != gameSession.activePawn->faction && !isInvalidCoordinate(gameSession.pawns[i].position)) {
            enemyUnits++;
        }
    }
    return enemyUnits;
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

// returns true when another movement has been scheduled
Boolean gameActionLogic_afterMove() {
    Boolean didScheduleMovement = false;
    Pawn *selectedPawn = gameSession.movement->targetPawn;
    StrCopy(gameSession.cpuActionText, "");
    // Check if flag was captured
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->inventory.carryingFlag && selectedPawn->inventory.flagOfFaction != gameSession.activePawn->faction) {
        gameSession.activePawn->inventory.carryingFlag = true;
        gameSession.activePawn->inventory.flagOfFaction = selectedPawn->inventory.flagOfFaction;
        selectedPawn->inventory.carryingFlag = false;
    }
    // Check if flag was returned to player's base
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->faction == gameSession.activePawn->faction && gameSession.activePawn->inventory.carryingFlag) {
        // Flag dissapears, enemy base dissapears, enemy ships join the players fleet
        int i;
        for (i = 0; i < gameSession.pawnCount; i++) {
            if (isInvalidCoordinate(gameSession.pawns[i].position)) {
                continue;
            }
            if (gameSession.pawns[i].faction == gameSession.activePawn->inventory.flagOfFaction) {
                gameSession.pawns[i].faction = gameSession.activePawn->faction;
                if (gameSession.pawns[i].type == PAWNTYPE_BASE) {
                    gameSession.pawns[i].position = (Coordinate){-1, -1};
                }
            }
        }
        gameSession.activePawn->inventory.carryingFlag = false;
        if (gameActionLogic_nonCapturedFlagsLeft(gameSession.activePawn->faction) > 0) {  // Still some flags left to capture
            FrmCustomAlert(GAME_ALERT_FLAGCAPTURED, NULL, NULL, NULL);
        } else {  // Game over, all flags captured
            FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_ALLFLAGSCAPTURED, NULL, NULL, NULL);
            gameSession_initialize();
        }
    }

    // if currentposition is on a base, move away from it
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE) {
        Coordinate finalCoordinate = movement_closestTileToTargetInRange(gameSession.activePawn, selectedPawn, gameSession.pawns, gameSession.pawnCount, false);
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
        
        if (gameSession.attackAnimation->targetPawn->type == PAWNTYPE_BASE) {
            int i;
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (isInvalidCoordinate(gameSession.pawns[i].position)) {
                    continue;
                }
                if (gameSession.pawns[i].faction == gameSession.attackAnimation->targetPawn->faction) {
                    gameSession.pawns[i].faction = gameSession.activePawn->faction;
                }
            }
            FrmCustomAlert(GAME_ALERT_BASEDESTROYED, NULL, NULL, NULL);
        }
        if (gameSession.attackAnimation->targetPawn->type == PAWNTYPE_SHIP && gameSession.attackAnimation->targetPawn->inventory.carryingFlag) {
            // return the lost flag to it's original base
            int i;
            gameSession.attackAnimation->targetPawn->inventory.carryingFlag = false;
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (gameSession.pawns[i].faction == gameSession.attackAnimation->targetPawn->inventory.flagOfFaction && gameSession.pawns[i].type == PAWNTYPE_BASE) {
                    gameSession.pawns[i].inventory.carryingFlag = true;
                }
            }
        }
    }

    // Check for game over if no enemy units left
    if (gameActionLogic_enemyUnitsLeft() == 0) {
        FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_TOTALDESTRUCTION, NULL, NULL, NULL);
        gameSession_initialize();
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