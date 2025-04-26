#include "cpuLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "movement.h"
#include "viewport.h"

typedef enum CPUStrategy {
    CPUSTRATEGY_DEFENDBASE,
    CPUSTRATEGY_CAPTUREFLAG,
    CPUSTRATEGY_ATTACK
} CPUStrategy;

static Pawn *cpuLogic_enemyWithStolenFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].faction != pawn->faction && allPawns[i].inventory.carryingFlag && allPawns[i].inventory.flagOfFaction == pawn->faction) {
            return &allPawns[i];
        }
    }
    return NULL;
}

static void cpuLogic_shuffledPawnsIndices(int *indices, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        indices[i] = i;
    }

    for (i = 0; i < totalPawnCount; i++) {
        int t;
        int j = random(0, totalPawnCount - 1);
        if (j == i) {
            continue;
        }
        t = indices[j];
        indices[j] = indices[i];
        indices[i] = t;
    }
}

/*static Pawn *cpuLogic_closestOtherFactionHomeBaseWithFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    Pawn *closestHomeBase = NULL;
    int closestDistance = 999;
    int indices[totalPawnCount];
    cpuLogic_shuffledPawnsIndices(indices, totalPawnCount);

    for (i = 0; i < totalPawnCount; i++) {
        int index = indices[i];
        if (!isInvalidCoordinate(allPawns[index].position) && allPawns[index].type == PAWNTYPE_BASE && allPawns[index].faction != pawn->faction && allPawns[index].inventory.carryingFlag) {
            int distance = movement_distance(pawn->position, allPawns[index].position);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestHomeBase = &allPawns[index];
            }
        }
    }
    return closestHomeBase;
}*/

static int cpuLogic_defenseValueForBase(Pawn *base, Pawn *allPawns, int totalPawnCount) {
    int i;
    int defenseValue = 0;
    int maxRange = GAMEMECHANICS_MAXTILEMOVERANGE * 2;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_SHIP && base->faction == allPawns[i].faction) {
            int distance = movement_distance(base->position, allPawns[i].position);
            int healthPercentage;
            if (distance >= maxRange) {
                continue;
            }
            /*FrmCustomAlert(GAME_ALERT_NOMOREACTIONS, NULL, NULL, NULL);
            healthPercentage = (float)(maxRange - distance) * 100.0 / (float)maxRange;
            defenseValue += ((float)allPawns[i].inventory.health * (float)healthPercentage / 100.0);
            */
           defenseValue += allPawns[i].inventory.health;
        }
    }

    return defenseValue;
}

static Pawn *cpuLogic_weakestOtherFactionHomeBaseWithFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    Pawn *weakestHomeBase = NULL;
    int weakestDefence = 9999;
    int indices[totalPawnCount];
    cpuLogic_shuffledPawnsIndices(indices, totalPawnCount);

    for (i = 0; i < totalPawnCount; i++) {
        int index = indices[i];
        if (!isInvalidCoordinate(allPawns[index].position) && allPawns[index].type == PAWNTYPE_BASE && allPawns[index].faction != pawn->faction && allPawns[index].inventory.carryingFlag) {
            int defenseValue = cpuLogic_defenseValueForBase(&allPawns[index], allPawns, totalPawnCount);
            if (defenseValue < weakestDefence) {
                weakestDefence = defenseValue;
                weakestHomeBase = &allPawns[index];
            }
        }
    }
    return weakestHomeBase;
}


static int cpuLogic_damageAssementForTile(Coordinate position, Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    int damage = 0;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_SHIP && pawn->faction != allPawns[i].faction) {
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
    int maxDamage = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount) != NULL ? 9999 : random(0, 5) >= 4 ? (pawn->inventory.health * 1.3) : 10;
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
            if (movement_shipAtTarget(candidateTile, allPawns, totalPawnCount) != NULL) {
                continue;
            }
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

static Pawn *cpuLogic_weakestEnemyInRange(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Boolean includeBases, Boolean unlimitedRange, int range) {
    int i;
    Pawn *weakestEnemy = NULL;
    int indices[totalPawnCount];
    cpuLogic_shuffledPawnsIndices(indices, totalPawnCount);

    for (i = 0; i < totalPawnCount; i++) {
        int index = indices[i];
        Boolean isShip = includeBases ? true : allPawns[index].type == PAWNTYPE_SHIP;
        if (!isInvalidCoordinate(allPawns[index].position) && allPawns[index].faction != pawn->faction && isShip) {
            int distance = unlimitedRange ? 0 : movement_distance(pawn->position, allPawns[index].position);
            if (distance <= range) {
                if (weakestEnemy == NULL) {
                    weakestEnemy = &allPawns[index];
                } else {
                    if (allPawns[index].inventory.health < weakestEnemy->inventory.health) {
                        weakestEnemy = &allPawns[index];
                    }
                }
            }
        }
    }
    return weakestEnemy;
}

static Boolean cpuLogic_attackIfInRange(Pawn *pawn, Pawn *target, CPUStrategyResult *updateStrategy) {
    int distanceToEnemy;
    if (pawn == NULL || target == NULL) {
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
    Pawn *homeBase = movement_homeBase(pawn, allPawns, totalPawnCount);
    Pawn *enemyInRangeOfHomeBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, false, false, (float)GAMEMECHANICS_MAXTILEMOVERANGE * 1.3);

    if (homeBase->inventory.carryingFlag == false) {  // flag was stolen! If enemy with flag is in range, attack, otherwise move to home base
        Pawn *enemyWithFlag = cpuLogic_enemyWithStolenFlag(pawn, allPawns, totalPawnCount);
        strategyResult.score += 50;
        if (enemyWithFlag != NULL) {
            if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
                Pawn *enemyHomeBase = movement_homeBase(enemyWithFlag, allPawns, totalPawnCount);
                if (enemyHomeBase != NULL) {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyHomeBase;
                }
            }
        }  // No else case, this means the flag was succesfully brought to the enemy base
    } else {
        if (enemyInRangeOfHomeBase != NULL) {
            strategyResult.score += 80;
            if (!cpuLogic_attackIfInRange(pawn, enemyInRangeOfHomeBase, &strategyResult)) {  // Attack if we can, if not, move towards enemy
                Boolean farAway = movement_distance(homeBase->position, pawn->position) > (float)GAMEMECHANICS_MAXTILEMOVERANGE * 1.3;
                if (!pawn->warped && farAway) {
                    strategyResult.score += 80;
                    strategyResult.CPUAction = CPUACTION_WARP;
                } else {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyInRangeOfHomeBase;
                }
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
        Pawn *homeBase = movement_homeBase(pawn, allPawns, totalPawnCount);
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = homeBase;
        strategyResult.score += 150;
    } else {
        int distance;
        Pawn *enemyHomeBase = cpuLogic_weakestOtherFactionHomeBaseWithFlag(pawn, allPawns, totalPawnCount);
        if (enemyHomeBase == NULL) {
            return strategyResult;
        }
        distance = movement_distance(pawn->position, enemyHomeBase->position);
        if (distance <= GAMEMECHANICS_MAXTILEMOVERANGE) {  // if we can capture the flag, do it
            strategyResult.score += 90;
            if (enemyHomeBase->inventory.carryingFlag) {
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = enemyHomeBase;
            } else if (!cpuLogic_attackIfInRange(pawn, enemyHomeBase, &strategyResult)) {
                strategyResult.score -= 120;
            }
        } else {  // if not, move towards enemy home base
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
            Pawn *enemyHomeBase = movement_homeBase(enemyWithFlag, allPawns, totalPawnCount);
            if (enemyHomeBase != NULL) {
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = enemyHomeBase;
            }
        }
    } else {
        Pawn *nearestEnemyShipOrBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, true, 1);
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
   //bestStrategy = strategyResult[CPUSTRATEGY_DEFENDBASE];
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
        case CPUACTION_NONE:
            drawhelper_drawText("NONE", (Coordinate){0, 30});
            break;
        case CPUACTION_WARP:
            drawhelper_drawText("WARP", (Coordinate){0, 30});
            break;
    }
    sleep(1000);
#endif
    return bestStrategy;
}
