#include "gameActionLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "MemoryMgr.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "hexgrid.h"
#include "level.h"
#include "models.h"
#include "movement.h"

GAMEACTIONLOGIC_SECTION
static UInt8 gameActionLogic_nonCapturedFlagsLeft(UInt8 faction, GameSession *session) {
    int i;
    UInt8 flagsLeft = 0;
    for (i = 0; i < session->level.pawnCount; i++) {
        // Count of enemies that are currently carrying a flag (including bases)
        // + Count of own ships that are currently carrying a flag (excluding home base)
        if (isInvalidCoordinate(session->level.pawns[i].position)) {
            continue;
        }
        if ((session->level.pawns[i].faction != faction && session->level.pawns[i].inventory.carryingFlag) ||
            (session->level.pawns[i].faction == faction && session->level.pawns[i].inventory.carryingFlag && session->level.pawns[i].type != PAWNTYPE_BASE)) {
            flagsLeft++;
        }
    }
    return flagsLeft;
}

GAMEACTIONLOGIC_SECTION
static void gameActionLogic_askForAfterGameOptions() {
    FrmCustomAlert(GAME_ALERT_ENDOFGAME, NULL, NULL, NULL) == 0 ? gameSession_reset(true) : gameSession_reset(false);
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_moveCameraToPawn(Pawn *pawn, GameSession *session) {
    session->cameraPawn = (Pawn){PAWNTYPE_SHIP, session->activePawn->position, (Inventory){-1, 0, 0, BASEACTION_NONE, false}, 0, 0, false, false};
    gameActionLogic_scheduleMovement(&session->cameraPawn, NULL, pawn->position, session);
}

GAMEACTIONLOGIC_SECTION
static void gameActionLogic_showScore(GameSession *session) {
    DmResID oldRank = scoring_rankForScore(scoring_loadSavedScore());
    DmResID newRank;
    Pawn oldPawn;
    Pawn navigationPawn;
    int i, humanCount = 0;
    oldPawn = *session->activePawn;
    navigationPawn = (Pawn){PAWNTYPE_SHIP, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETLEFT, 0}, (Inventory){-1, 0, 0, BASEACTION_NONE, false}, 0, 0, false, false};
    for (i = 0; i < session->factionCount; i++) {
        if (session->factions[i].human) {
            humanCount++;
        }
    }
    if (humanCount > 1) {
        // No score reporting in multiplayer games
        gameSession_reset(false);
        return;
    }
    session->menuScreenType = MENUSCREEN_SCORE;
    session->drawingState.shouldRedrawBackground = true;
    session->drawingState.shouldRedrawHeader = true;
    session->activePawn->type = PAWNTYPE_SHIP;
    scoring_saveScore(session->level.scores, session->activePawn->faction);

    newRank = scoring_rankForScore(scoring_loadSavedScore());
    if (newRank != oldRank) {
        MemHandle resourceHandle = DmGetResource(strRsc, newRank);
        Char *rankText = (char *)MemHandleLock(resourceHandle);
        FrmCustomAlert(GAME_ALERT_PROMOTED, rankText, "", "");
        DmReleaseResource(resourceHandle);
    }
    level_addScorePawns(&session->level, session->activePawn->faction);

    session->activePawn = level_pawnAtTile(oldPawn.position, &session->level);
    if (session->activePawn == NULL) {
        level_addPawn(oldPawn, &session->level);

        session->activePawn = level_pawnAtTile(oldPawn.position, &session->level);
    }
    level_addPawn(navigationPawn, &session->level);
    gameActionLogic_moveCameraToPawn(&navigationPawn, session);
    session->activePawn = level_pawnAtTile(navigationPawn.position, &session->level);
}

GAMEACTIONLOGIC_SECTION
Boolean gameActionLogic_humanShipsLeft(GameSession *session) {
    int i;
    for (i = 0; i < session->level.pawnCount; i++) {
        if (session->factions[session->level.pawns[i].faction].human && session->level.pawns[i].type == PAWNTYPE_SHIP && !isInvalidCoordinate(session->level.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

GAMEACTIONLOGIC_SECTION
static Boolean gameActionLogic_enemyShipsLeft(GameSession *session) {
    int i;
    for (i = 0; i < session->level.pawnCount; i++) {
        if (session->level.pawns[i].type == PAWNTYPE_SHIP && session->level.pawns[i].faction != session->activePawn->faction && !isInvalidCoordinate(session->level.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_clearSceneAnimation(GameSession *session) {
    if (session->sceneAnimation == NULL) {
        return;
    }
    MemPtrFree(session->sceneAnimation);
    session->sceneAnimation = NULL;
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_clearMovement(GameSession *session) {
    if (session->movement != NULL) {
        if (session->movement->trajectory.tileCoordinates != NULL) {
            MemPtrFree(session->movement->trajectory.tileCoordinates);
            session->movement->trajectory.tileCoordinates = NULL;
        }
        MemPtrFree(session->movement);
        session->movement = NULL;
    }
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_scheduleWarp(Pawn *sourcePawn, Coordinate target, GameSession *session) {
    session->warpAnimation.pawn = sourcePawn;
    session->warpAnimation.isWarping = true;
    session->warpAnimation.currentPosition = sourcePawn->position;
    session->warpAnimation.endPosition = target;
    session->warpAnimation.launchTimestamp = TimGetTicks();
    session->warpAnimation.shipVisible = true;
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_scheduleShockwave(Pawn *basePawn, GameSession *session) {
    int i, affectedPawnCount = 0;
    gameActionLogic_clearShockwave(session);
    session->shockWaveAnimation = (ShockWaveAnimation *)MemPtrNew(sizeof(ShockWaveAnimation));
    MemSet(session->shockWaveAnimation, sizeof(ShockWaveAnimation), 0);
    session->shockWaveAnimation->launchTimestamp = TimGetTicks();
    session->shockWaveAnimation->basePawn = basePawn;
    session->shockWaveAnimation->affectedPawnIndices = (int *)MemPtrNew(sizeof(int) * session->level.pawnCount);
    session->shockWaveAnimation->pawnOriginalPositions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * session->level.pawnCount);
    session->shockWaveAnimation->pawnIntermediatePositions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * session->level.pawnCount);

    for (i = 0; i < session->level.pawnCount; i++) {
        if (!isInvalidCoordinate(session->level.pawns[i].position) && session->level.pawns[i].type == PAWNTYPE_SHIP && movement_distance(basePawn->position, session->level.pawns[i].position) < GAMEMECHANICS_SHOCKWAVERANGE - 1) {
            session->shockWaveAnimation->affectedPawnIndices[affectedPawnCount] = i;
            session->shockWaveAnimation->pawnOriginalPositions[affectedPawnCount] = session->level.pawns[i].position;
            session->level.pawns[i].position = movement_positionAwayFrom(basePawn->position, &session->level.pawns[i], session->level.pawns, session->level.pawnCount, GAMEMECHANICS_SHOCKWAVERANGE);
            affectedPawnCount++;
        }
    }
    session->shockWaveAnimation->affectedPawnCount = affectedPawnCount;
    MemPtrResize(session->shockWaveAnimation->affectedPawnIndices, sizeof(int) * affectedPawnCount);
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_scheduleMovement(Pawn *sourcePawn, Pawn *targetPawn, Coordinate selectedTile, GameSession *session) {
    gameActionLogic_clearMovement(session);

    session->movement = (Movement *)MemPtrNew(sizeof(Movement));
    MemSet(session->movement, sizeof(Movement), 0);
    session->movement->launchTimestamp = TimGetTicks();
    session->movement->targetPawn = targetPawn;

    if (sourcePawn == &session->cameraPawn) {
        session->movement->trajectory.tileCoordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * 2);
        session->movement->trajectory.tileCount = 2;
        session->movement->trajectory.tileCoordinates[0] = (Coordinate){session->cameraPawn.position.x, session->cameraPawn.position.y};
        session->movement->trajectory.tileCoordinates[1] = selectedTile;
    } else {
        session->movement->trajectory = movement_trajectoryBetween((Coordinate){sourcePawn->position.x, sourcePawn->position.y}, selectedTile);
    }
    session->movement->pawnPosition = hexgrid_tileCenterPosition(sourcePawn->position);
    session->movement->pawn = sourcePawn;
    sourcePawn->position = selectedTile;
}

GAMEACTIONLOGIC_SECTION
static Boolean gameActionLogic_baseOnPosition(Coordinate position, GameSession *session) {
    int i;
    for (i = 0; i < session->level.pawnCount; i++) {
        if (!isInvalidCoordinate(session->level.pawns[i].position) && isEqualCoordinate(session->level.pawns[i].position, position) && session->level.pawns[i].type == PAWNTYPE_BASE) {
            return true;
        }
    }
    return false;
}

GAMEACTIONLOGIC_SECTION
static Boolean gameActionLogic_checkForGameOver(GameSession *session) {
    if (!session->continueCPUPlay && !gameActionLogic_humanShipsLeft(session)) {
        if (FrmCustomAlert(GAME_ALERT_CPUCONTINUEPLAYING, NULL, NULL, NULL) != 0) {  // Do not continue playing
            gameActionLogic_showScore(session);
            return true;
        } else {
            session->continueCPUPlay = true;
        }
    }
    return false;
}

GAMEACTIONLOGIC_SECTION
static void gameActionLogic_removeBase(int baseFaction, int newFaction, GameSession *session) {
    int i;
    for (i = 0; i < session->level.pawnCount; i++) {
        if (session->level.pawns[i].faction == baseFaction && session->level.pawns[i].type == PAWNTYPE_BASE) {
            Coordinate activePawnPosition = session->activePawn->position;
            level_removePawnAtIndex(i, &session->level);
            session->activePawn = level_pawnTypeAtTile(activePawnPosition, &session->level, PAWNTYPE_SHIP, true);
            break;
        }
    }
    // For all remaining ships of the removed faction, return any flags they carry AND update them to the new faction
    for (i = 0; i < session->level.pawnCount; i++) {
        if (session->level.pawns[i].faction == baseFaction && session->level.pawns[i].type == PAWNTYPE_SHIP) {
            level_returnFlagFromPawnToOriginalBase(&session->level.pawns[i], &session->level);

            session->level.scores[newFaction].shipsCaptured[baseFaction]++;
            session->level.pawns[i].faction = newFaction;
        }
    }
}

// returns true when another movement has been scheduled
GAMEACTIONLOGIC_SECTION
Boolean gameActionLogic_afterMove(GameSession *session) {
    Boolean didScheduleMovement = false;
    Pawn *selectedPawn = session->movement->targetPawn;

    if (session->movement->pawn != NULL && session->movement->pawn == &session->cameraPawn) {
        gameActionLogic_clearMovement(session);
        session->state = GAMESTATE_DEFAULT;
        return false;
    }

    StrCopy(session->cpuActionText, "");
    // Check if flag was captured
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->inventory.carryingFlag && !session->activePawn->inventory.carryingFlag && selectedPawn->inventory.flagOfFaction != session->activePawn->faction) {
        session->activePawn->inventory.carryingFlag = true;
        session->activePawn->inventory.flagOfFaction = selectedPawn->inventory.flagOfFaction;
        selectedPawn->inventory.carryingFlag = false;
        session->level.scores[session->activePawn->faction].flagsStolen++;
    }
    // Check if flag was returned to player's base
    if (selectedPawn != NULL && selectedPawn->type == PAWNTYPE_BASE && selectedPawn->faction == session->activePawn->faction && session->activePawn->inventory.carryingFlag) {
        // Flag dissapears, enemy base dissapears, enemy ships join the players fleet
        session->activePawn->inventory.carryingFlag = false;
        session->level.scores[session->activePawn->faction].flagsCaptured[session->activePawn->inventory.flagOfFaction]++;
        gameActionLogic_removeBase(session->activePawn->inventory.flagOfFaction, session->activePawn->faction, session);
        if (gameActionLogic_nonCapturedFlagsLeft(session->activePawn->faction, session) > 0) {  // Still some flags left to capture
            FrmCustomAlert(GAME_ALERT_FLAGCAPTURED, NULL, NULL, NULL);
            if (gameActionLogic_checkForGameOver(session)) {
                return false;
            }
        } else {  // Game over, all flags captured
            if (session->factions[session->activePawn->faction].human) {
                FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_ALLFLAGSCAPTURED, NULL, NULL, NULL);
                gameActionLogic_showScore(session);
            } else {
                FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_CPUALLFLAGSCAPTURED, NULL, NULL, NULL);
                gameActionLogic_askForAfterGameOptions();
            }
            return false;
        }
    }

    // if currentposition is on a base, move away from it
    if (session->activePawn->type != PAWNTYPE_BASE && gameActionLogic_baseOnPosition(session->activePawn->position, session)) {
        Coordinate finalCoordinate = movement_closestTileToTargetInRange(session->activePawn, session->activePawn->position, session->level.pawns, session->level.pawnCount, false);
        gameActionLogic_scheduleMovement(session->activePawn, NULL, finalCoordinate, session);
        didScheduleMovement = true;
    } else {
        gameActionLogic_clearMovement(session);
        session->state = GAMESTATE_DEFAULT;
    }
    return didScheduleMovement;
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_afterExplosion(GameSession *session) {
    // Check for game over if no enemy units left
    if (!gameActionLogic_enemyShipsLeft(session)) {
        if (session->factions[session->activePawn->faction].human) {
            FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_TOTALDESTRUCTION, NULL, NULL, NULL);
            gameActionLogic_showScore(session);
        } else {
            FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_CPUTOTALDESTRUCTION, NULL, NULL, NULL);
            gameActionLogic_askForAfterGameOptions();
        }
        return;
    }

    if (gameActionLogic_checkForGameOver(session)) {
        return;
    }
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_afterAttack(GameSession *session) {
    StrCopy(session->cpuActionText, "");
    // Update inventory stats
    session->attackAnimation->targetPawn->inventory.health -= session->attackAnimation->healthImpact;
    if (session->targetSelectionType == TARGETSELECTIONTYPE_TORPEDO) {
        session->activePawn->inventory.torpedoCount--;
    }

    if (session->attackAnimation->targetPawn->inventory.health <= 0) {
        Coordinate activePawnPosition;
        int oldFaction = session->attackAnimation->targetPawn->faction;

        if (session->attackAnimation->targetPawn->type == PAWNTYPE_BASE) {
            gameActionLogic_removeBase(oldFaction, session->activePawn->faction, session);
            session->level.scores[session->activePawn->faction].basesDestroyed[oldFaction] = true;
            FrmCustomAlert(GAME_ALERT_BASEDESTROYED, NULL, NULL, NULL);
        } else {
            level_returnFlagFromPawnToOriginalBase(session->attackAnimation->targetPawn, &session->level);
            session->level.scores[session->activePawn->faction].shipsDestroyed[oldFaction]++;
            activePawnPosition = session->activePawn->position;
            level_removePawn(session->attackAnimation->targetPawn, &session->level);
            session->activePawn = level_pawnAtTile(activePawnPosition, &session->level);
            session->attackAnimation->targetPawn = NULL;
        }
    }
}

GAMEACTIONLOGIC_SECTION
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

GAMEACTIONLOGIC_SECTION
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

GAMEACTIONLOGIC_SECTION
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

GAMEACTIONLOGIC_SECTION
void gameActionLogic_clearShockwave(GameSession *session) {
    if (session->shockWaveAnimation != NULL) {
        if (session->shockWaveAnimation->affectedPawnIndices != NULL) {
            MemPtrFree(session->shockWaveAnimation->affectedPawnIndices);
            session->shockWaveAnimation->affectedPawnIndices = NULL;
        }
        if (session->shockWaveAnimation->pawnIntermediatePositions != NULL) {
            MemPtrFree(session->shockWaveAnimation->pawnIntermediatePositions);
            session->shockWaveAnimation->pawnIntermediatePositions = NULL;
        }
        if (session->shockWaveAnimation->pawnOriginalPositions != NULL) {
            MemPtrFree(session->shockWaveAnimation->pawnOriginalPositions);
            session->shockWaveAnimation->pawnOriginalPositions = NULL;
        }
        MemPtrFree(session->shockWaveAnimation);
        session->shockWaveAnimation = NULL;
    }
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_clearAttack(GameSession *session) {
    if (session->attackAnimation != NULL) {
        if (session->attackAnimation->lines != NULL) {
            MemPtrFree(session->attackAnimation->lines);
            session->attackAnimation->lines = NULL;
        }
        MemPtrFree(session->attackAnimation);
        session->attackAnimation = NULL;
    }
}

GAMEACTIONLOGIC_SECTION
void gameActionLogic_scheduleAttack(Pawn *targetPawn, Coordinate selectedTile, TargetSelectionType attackType, GameSession *session) {
    gameActionLogic_clearAttack(session);
    if (targetPawn != NULL) {
        session->targetSelectionType = attackType;
        session->attackAnimation = (AttackAnimation *)MemPtrNew(sizeof(AttackAnimation));
        MemSet(session->attackAnimation, sizeof(AttackAnimation), 0);
        session->attackAnimation->torpedoPosition = (Coordinate){-1, -1};
        session->attackAnimation->explosionPosition = (Coordinate){-1, -1};
        session->attackAnimation->launchTimestamp = TimGetTicks();
        session->attackAnimation->target = selectedTile;
        session->attackAnimation->targetPawn = targetPawn;
        session->attackAnimation->healthImpact = gameActionLogic_healthImpact(session->activePawn->position, selectedTile, session->targetSelectionType);
        session->attackAnimation->durationSeconds = gameActionLogic_attackDuration(session->activePawn->position, selectedTile, session->targetSelectionType);
        session->activePawn->orientation = movement_orientationBetween(session->activePawn->position, selectedTile);
    } else {
        session->state = GAMESTATE_DEFAULT;
    }
}
