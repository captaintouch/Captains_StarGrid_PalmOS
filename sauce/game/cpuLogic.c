#include "cpuLogic.h"

#include "../constants.h"
#include "movement.h"

typedef enum CPUStrategy {
    CPUSTRATEGY_DEFENDBASE,
    CPUSTRATEGY_CAPTUREFLAG,
    CPUSTRATEGY_ATTACK
} CPUStrategy;

static Pawn *cpuLogic_closestOtherFactionHomeBaseWithFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    Pawn *closestHomeBase = NULL;
    int closestDistance = 999;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_BASE && allPawns[i].faction != pawn->faction && allPawns[i].inventory.carryingFlag) {
            int distance = movement_distance(pawn->position, allPawns[i].position);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestHomeBase = &allPawns[i];
            }
        }
    }
    return closestHomeBase;
}

static Pawn *cpuLogic_homeBase(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_BASE && allPawns[i].faction == pawn->faction) {
            return &allPawns[i];
        }
    }
    return NULL;
}

/*static Boolean cpuLogic_closeToHomeBase(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    Pawn *homeBase = cpuLogic_homeBase(pawn, allPawns, totalPawnCount);
    if (homeBase == NULL) {
        return false;
    }
    return movement_distance(pawn->position, homeBase->position) <= GAMEMECHANICS_MAXTILEMOVERANGE;
}*/

static Pawn *cpuLogic_weakestEnemyInRange(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Boolean includeBases, Boolean unlimitedRange) {
    int i;
    Pawn *weakestEnemy = NULL;
    for (i = 0; i < totalPawnCount; i++) {
        Boolean isShip = includeBases ? true : allPawns[i].type == PAWNTYPE_SHIP;
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].faction != pawn->faction && isShip) {
            int distance = unlimitedRange ? 0 : movement_distance(pawn->position, allPawns[i].position);
            if (distance <= fmax(GAMEMECHANICS_MAXTILEPHASERRANGE, GAMEMECHANICS_MAXTILETORPEDORANGE)) {
                if (weakestEnemy == NULL) {
                    weakestEnemy = &allPawns[i];
                } else {
                    if (allPawns[i].inventory.health < weakestEnemy->inventory.health) {
                        weakestEnemy = &allPawns[i];
                    }
                }
            }
        }
    }
    return weakestEnemy;
}

static Pawn *cpuLogic_enemyWithStolenFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].faction != pawn->faction && allPawns[i].inventory.carryingFlag && allPawns[i].inventory.flagOfFaction == pawn->faction) {
            return &allPawns[i];
        }
    }
    return NULL;
}

static Boolean cpuLogic_attackIfInRange(Pawn *pawn, Pawn *target, CPUStrategyResult *updateStrategy) {
    int distanceToEnemy;
    if (pawn == NULL || target == NULL) {
        return false;
    }
    distanceToEnemy = movement_distance(pawn->position, target->position);
    if (distanceToEnemy <= GAMEMECHANICS_MAXTILETORPEDORANGE) {  // When in range, attack with torpedoes
        updateStrategy->CPUAction = CPUACTION_TORPEDOATTACK;
        updateStrategy->target = target;
        return true;
    } else if (distanceToEnemy <= GAMEMECHANICS_MAXTILEPHASERRANGE) {
        updateStrategy->CPUAction = CPUACTION_PHASERATTACK;
        updateStrategy->target = target;
        return true;
    } else {
        return false;
    }
}

static CPUStrategyResult cpuLogic_defendBaseStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    CPUStrategyResult strategyResult = {70, CPUACTION_NONE, NULL};
    Pawn *homeBase = cpuLogic_homeBase(pawn, allPawns, totalPawnCount);
    Pawn *enemyInRangeOfHomeBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, false, false);

    if (homeBase->inventory.carryingFlag == false) {  // flag was stolen! If enemy with flag is in range, attack, otherwise move to home base
        Pawn *enemyWithFlag = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount);
        strategyResult.score += 50;
        if (enemyWithFlag != NULL) {
            if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
                Pawn *enemyHomeBase = cpuLogic_homeBase(enemyWithFlag, allPawns, totalPawnCount);
                if (enemyHomeBase != NULL) {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyHomeBase;
                }
            }
        }  // No else case, this means the flag was succesfully brought to the enemy base
    } else {
        if (enemyInRangeOfHomeBase != NULL) {
            strategyResult.score += 50;
            if (!cpuLogic_attackIfInRange(pawn, enemyInRangeOfHomeBase, &strategyResult)) {  // Attack if we can, if not, move towards enemy
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = enemyInRangeOfHomeBase;
            }
        } else {  // no enemies near base, go easy on this strategy
            strategyResult.score -= 50;
        }
    }

    return strategyResult;
}

static CPUStrategyResult cpuLogic_captureTheFlagStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    CPUStrategyResult strategyResult = {60, CPUACTION_NONE, NULL};
    Pawn *enemyHomeBase = cpuLogic_closestOtherFactionHomeBaseWithFlag(pawn, allPawns, totalPawnCount);
    int distance;
    if (enemyHomeBase == NULL) {
        return (CPUStrategyResult){0, CPUACTION_NONE, NULL};
    }
    distance = movement_distance(pawn->position, enemyHomeBase->position);
    if (pawn->inventory.carryingFlag) {
        Pawn *homeBase = cpuLogic_homeBase(pawn, allPawns, totalPawnCount);
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = homeBase;
        strategyResult.score += 150;
    } else if (distance <= GAMEMECHANICS_MAXTILEMOVERANGE) {  // if we can capture the flag, do it
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = enemyHomeBase;
        strategyResult.score += 50;
    } else {  // if not, move towards enemy home base
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = enemyHomeBase;
    }

    return strategyResult;
}

static CPUStrategyResult cpuLogic_attackStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    CPUStrategyResult strategyResult = {40, CPUACTION_NONE, NULL};

    Pawn *enemyWithFlag = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount);
    if (enemyWithFlag != NULL) {
        strategyResult.score += 50;
        if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
            Pawn *enemyHomeBase = cpuLogic_homeBase(enemyWithFlag, allPawns, totalPawnCount);
            if (enemyHomeBase != NULL) {
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = enemyHomeBase;
            }
        }
    } else {
        Pawn *nearestEnemyShipOrBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, true);
        if (nearestEnemyShipOrBase != NULL) {
            strategyResult.score += 50;
            if (!cpuLogic_attackIfInRange(pawn, nearestEnemyShipOrBase, &strategyResult)) {  // Attack if we can, if not, move to enemy
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = nearestEnemyShipOrBase;
            }
        }
    }

    return strategyResult;
}

CPUStrategyResult cpuLogic_getStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    CPUStrategyResult bestStrategy;
    CPUStrategyResult strategyResult[3];
    if (isInvalidCoordinate(pawn->position)) {
        return (CPUStrategyResult){CPUACTION_NONE, NULL};
    }

    strategyResult[CPUSTRATEGY_DEFENDBASE] = cpuLogic_defendBaseStrategy(pawn, allPawns, totalPawnCount);
    strategyResult[CPUSTRATEGY_CAPTUREFLAG] = cpuLogic_captureTheFlagStrategy(pawn, allPawns, totalPawnCount);
    strategyResult[CPUSTRATEGY_ATTACK] = cpuLogic_attackStrategy(pawn, allPawns, totalPawnCount);

    bestStrategy = strategyResult[0];
    for (i = 1; i < 3; i++) {
        if (strategyResult[i].CPUAction == CPUACTION_NONE) {
            strategyResult[i].score = -1;
        }
        if (strategyResult[i].score > bestStrategy.score) {
            bestStrategy = strategyResult[i];
        }
    }
    return bestStrategy;
}