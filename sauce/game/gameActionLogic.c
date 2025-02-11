#include "gameActionLogic.h"

#include "../constants.h"
#include "gamesession.h"
#include "models.h"

static UInt8 gameActionLogic_nonCapturedFlagsLeft(UInt8 faction) {
    int i;
    UInt8 flagsLeft = 0;
    for (i = 0; i < gameSession.pawnCount; i++) {
        // Count of enemies that are currently carrying a flag (including bases)
        // + Count of own ships that are currently carrying a flag (excluding home base)
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

void gameActionLogic_afterMove() {
    Pawn *selectedPawn = gameSession.movement->targetPawn;
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
}

void gameActionLogic_afterAttack() {
    // Update health stats
    gameSession.attackAnimation->targetPawn->inventory.health -= gameSession.attackAnimation->healthImpact;
    if (gameSession.attackAnimation->targetPawn->inventory.health <= 0) {
        gameSession.attackAnimation->targetPawn->inventory.health = 0;
        gameSession.attackAnimation->targetPawn->position = (Coordinate){-1, -1};

        if (gameSession.attackAnimation->targetPawn->type == PAWNTYPE_BASE) {
            int i;
            for (i = 0; i < gameSession.pawnCount; i++) {
                if (gameSession.pawns[i].faction == gameSession.attackAnimation->targetPawn->faction) {
                    gameSession.pawns[i].faction = gameSession.activePawn->faction;
                }
            }
            FrmCustomAlert(GAME_ALERT_BASEDESTROYED, NULL, NULL, NULL);
        }
    }

    // Check for game over if no enemy units left
    if (gameActionLogic_enemyUnitsLeft() == 0) {
        FrmCustomAlert(GAME_ALERT_GAMECOMPLETE_TOTALDESTRUCTION, NULL, NULL, NULL);
        gameSession_initialize();
    }
}