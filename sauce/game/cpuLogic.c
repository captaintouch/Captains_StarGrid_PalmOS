#include "cpuLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "mathIsFun.h"
#include "movement.h"
#include "hexgrid.h"
#include "viewport.h"

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

static int cpuLogic_damageAssementForTile(Coordinate position, Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    int damage = 0;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_SHIP && pawn->faction != allPawns[i].faction && !allPawns[i].cloaked) {
            int distance = movement_distance(position, allPawns[i].position);
            if (allPawns[i].inventory.torpedoCount > 0 && distance <= GAMEMECHANICS_MAXTILETORPEDORANGE) {
                damage += GAMEMECHANICS_MAXIMPACTTORPEDO;
            } else if (distance <= GAMEMECHANICS_MAXTILEPHASERRANGE) {
                damage += GAMEMECHANICS_MAXIMPACTPHASER;
            }
        }
    }
    return damage;
}

// This function is used to find a safe position for the pawn to move to, avoiding enemy ships
static Coordinate cpuLogic_safePosition(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Pawn *target) {
    int maxRange = GAMEMECHANICS_MAXTILEMOVERANGE;
    int dx, dy;
    int minDistance = 9999;
    int maxDamage = random(0, 5) >= 4 ? pawn->inventory.health - 5 : 10;
    Coordinate targetPosition, safePosition;
    if (target == NULL || isInvalidCoordinate(target->position)) {
        return (Coordinate){-1, -1};
    }
    targetPosition = movement_closestTileToTargetInRange(pawn, target->position, allPawns, totalPawnCount, true);
    safePosition = targetPosition;
    for (dx = -maxRange; dx <= maxRange; dx++) {
        for (dy = -maxRange; dy <= maxRange; dy++) {
            Coordinate candidateTile = {pawn->position.x + dx, pawn->position.y + dy};
            int possibleDamage = cpuLogic_damageAssementForTile(candidateTile, pawn, allPawns, totalPawnCount);
            #ifdef DEBUG
            drawhelper_drawTextWithValue("", possibleDamage, viewport_convertedCoordinate(hexgrid_tileCenterPosition(candidateTile)));
            #endif
            if (!isInvalidCoordinate(candidateTile) && possibleDamage <= maxDamage && movement_distance(pawn->position, candidateTile) <= maxRange) {
                int distance = movement_distance(candidateTile, targetPosition);
                if (distance < minDistance) {
                    minDistance = distance;
                    safePosition = candidateTile;
                }
            }
        }
    }
    return safePosition;
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

static Pawn *cpuLogic_weakestEnemyInRange(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Boolean includeBases, Boolean unlimitedRange) {
    int i;
    Pawn *weakestEnemy = NULL;
    for (i = 0; i < totalPawnCount; i++) {
        Boolean isShip = includeBases ? true : allPawns[i].type == PAWNTYPE_SHIP;
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].faction != pawn->faction && isShip && !allPawns[i].cloaked) {
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
    if (pawn->cloaked) {  // can't attack while cloaked
        return false;
    }
    distanceToEnemy = movement_distance(pawn->position, target->position);
    if (distanceToEnemy <= GAMEMECHANICS_MAXTILETORPEDORANGE && pawn->inventory.torpedoCount > 0) {  // When in range, attack with torpedoes
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

static CPUStrategyResult cpuLogic_defendBaseStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-30, 30), CPUACTION_NONE, NULL};
    Pawn *homeBase = cpuLogic_homeBase(pawn, allPawns, totalPawnCount);
    Pawn *enemyInRangeOfHomeBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, false, false);

    if (homeBase->inventory.carryingFlag == false) {  // flag was stolen! If enemy with flag is in range, attack, otherwise move to home base
        Pawn *enemyWithFlag = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount);
        strategyResult.score += 50;
        if (enemyWithFlag != NULL) {
            if (pawn->cloaked) {
                strategyResult.CPUAction = CPUACTION_CLOAK;
            } else if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
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
            if (pawn->cloaked) {
                strategyResult.CPUAction = CPUACTION_CLOAK;                                         // Decloak so we are go for attack on the next turn
            } else if (!cpuLogic_attackIfInRange(pawn, enemyInRangeOfHomeBase, &strategyResult)) {  // Attack if we can, if not, move towards enemy
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = enemyInRangeOfHomeBase;
            }
        } else {  // no enemies near base, go easy on this strategy
            strategyResult.score -= 50;
        }
    }

    return strategyResult;
}

static CPUStrategyResult cpuLogic_captureTheFlagStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-30, 30), CPUACTION_NONE, NULL};

    if (pawn->inventory.carryingFlag) {
        Pawn *homeBase = cpuLogic_homeBase(pawn, allPawns, totalPawnCount);
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = homeBase;
        strategyResult.score += 150;
    } else {
        int distance;
        Pawn *enemyHomeBase = cpuLogic_closestOtherFactionHomeBaseWithFlag(pawn, allPawns, totalPawnCount);
        if (enemyHomeBase == NULL) {
            return strategyResult;
        }
        distance = movement_distance(pawn->position, enemyHomeBase->position);
        if (pawn->cloaked && distance <= GAMEMECHANICS_MAXTILEMOVERANGE) {  // if we are cloaked, and close to the target, decloak
            strategyResult.score += 50;
            strategyResult.CPUAction = CPUACTION_CLOAK;                                 // Decloak so we are go for capture on the next turn
        } else if (!pawn->cloaked && distance >= GAMEMECHANICS_MAXTILEMOVERANGE * 2) {  // if we are not cloaked, and far away from the target, activate cloak
            strategyResult.score += 50;
            strategyResult.CPUAction = CPUACTION_CLOAK;
        } else if (!pawn->cloaked && distance <= GAMEMECHANICS_MAXTILEMOVERANGE) {  // if we can capture the flag, do it
            strategyResult.CPUAction = CPUACTION_MOVE;
            strategyResult.target = enemyHomeBase;
            strategyResult.score += 90;
        } else if (enemyHomeBase != NULL) {  // if not, move towards enemy home base
            strategyResult.CPUAction = CPUACTION_MOVE;
            strategyResult.target = enemyHomeBase;
        }
    }

    return strategyResult;
}

static CPUStrategyResult cpuLogic_attackStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-40, 40), CPUACTION_NONE, NULL};

    Pawn *enemyWithFlag = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount);
    if (enemyWithFlag != NULL) {
        strategyResult.score += 50;
        if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
            Pawn *enemyHomeBase = cpuLogic_homeBase(enemyWithFlag, allPawns, totalPawnCount);
            if (enemyHomeBase != NULL) {
                if (pawn->cloaked) {
                    strategyResult.CPUAction = CPUACTION_CLOAK;
                } else {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyHomeBase;
                }
            }
        }
    } else {
        Pawn *nearestEnemyShipOrBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, true);
        if (nearestEnemyShipOrBase != NULL) {
            strategyResult.score += 50;
            if (pawn->cloaked) {
                strategyResult.CPUAction = CPUACTION_CLOAK;
            } else if (!cpuLogic_attackIfInRange(pawn, nearestEnemyShipOrBase, &strategyResult)) {  // Attack if we can, if not, move to enemy
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = nearestEnemyShipOrBase;
            }
        }
    }

    return strategyResult;
}

CPUStrategyResult cpuLogic_getStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, CPUFactionProfile factionProfile) {
    int i;
    CPUStrategyResult bestStrategy;
    CPUStrategyResult strategyResult[3];
    if (isInvalidCoordinate(pawn->position)) {
        return (CPUStrategyResult){CPUACTION_NONE, NULL};
    }

    strategyResult[CPUSTRATEGY_DEFENDBASE] = cpuLogic_defendBaseStrategy(pawn, allPawns, totalPawnCount, factionProfile.defendBasePriority);
    strategyResult[CPUSTRATEGY_CAPTUREFLAG] = cpuLogic_captureTheFlagStrategy(pawn, allPawns, totalPawnCount, factionProfile.captureFlagPriority);
    strategyResult[CPUSTRATEGY_ATTACK] = cpuLogic_attackStrategy(pawn, allPawns, totalPawnCount, factionProfile.attackPriority);

#ifdef DEBUG
    drawhelper_drawTextWithValue("DEFBASE:", strategyResult[CPUSTRATEGY_DEFENDBASE].score, (Coordinate){0, 0});
    drawhelper_drawTextWithValue("CTF:", strategyResult[CPUSTRATEGY_CAPTUREFLAG].score, (Coordinate){0, 10});
    drawhelper_drawTextWithValue("ATTACK:", strategyResult[CPUSTRATEGY_ATTACK].score, (Coordinate){0, 20});
#endif

    bestStrategy = strategyResult[0];
    for (i = 1; i < 3; i++) {
        if (strategyResult[i].CPUAction == CPUACTION_NONE) {
            strategyResult[i].score = -999;
        }
        if (strategyResult[i].score > bestStrategy.score) {
            bestStrategy = strategyResult[i];
        }
    }
    bestStrategy.targetPosition = cpuLogic_safePosition(pawn, allPawns, totalPawnCount, bestStrategy.target);

#ifdef DEBUG
    switch (bestStrategy.CPUAction) {
        case CPUACTION_MOVE:
            drawhelper_drawTextWithValue("MOVE X:", bestStrategy.target->position.x, (Coordinate){0, 30});
            drawhelper_drawTextWithValue("Y:", bestStrategy.target->position.y, (Coordinate){45, 30});
            break;
        case CPUACTION_PHASERATTACK:
        case CPUACTION_TORPEDOATTACK:
            drawhelper_drawText("ATTACK", (Coordinate){0, 30});
            break;
        case CPUACTION_CLOAK:
            drawhelper_drawText("CLOAK", (Coordinate){0, 30});
            break;
        case CPUACTION_NONE:
            drawhelper_drawText("NONE", (Coordinate){0, 30});
            break;
    }
    sleep(1000);
#endif
    return bestStrategy;
}
