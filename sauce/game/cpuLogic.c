#include "cpuLogic.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "movement.h"
#include "pawn.h"
#include "viewport.h"

typedef enum CPUStrategy {
    CPUSTRATEGY_DEFENDBASE,
    CPUSTRATEGY_CAPTUREFLAG,
    CPUSTRATEGY_ATTACK,
    CPUSTRATEGY_PROVIDEBACKUP
} CPUStrategy;

CPULOGIC_SECTION
static Pawn *cpuLogic_pawnWithStolenFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int maxDistance, Boolean enemy) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        Boolean isExpectedFaction = enemy ? allPawns[i].inventory.flagOfFaction == pawn->faction : allPawns[i].faction == pawn->faction;
        if (pawn->type == PAWNTYPE_SHIP && !isInvalidCoordinate(allPawns[i].position) && allPawns[i].faction != pawn->faction && allPawns[i].inventory.carryingFlag && isExpectedFaction) {
            if (maxDistance > 0 && movement_distance(pawn->position, allPawns[i].position) <= maxDistance) {
                return &allPawns[i];
            } else {
                return &allPawns[i];
            }
        }
    }
    return NULL;
}

CPULOGIC_SECTION
static int cpuLogic_defenseValueForPawn(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    int defenseValue = 0;
    int maxRange = GAMEMECHANICS_MAXTILEMOVERANGE * 2;
    for (i = 0; i < totalPawnCount; i++) {
        if (!isInvalidCoordinate(allPawns[i].position) && allPawns[i].type == PAWNTYPE_SHIP && pawn->faction == allPawns[i].faction) {
            int distance = movement_distance(pawn->position, allPawns[i].position);
            int healthPercentage;
            if (distance >= maxRange) {
                continue;
            }
            healthPercentage = (float)(maxRange - distance) * 100.0 / (float)maxRange;
            defenseValue += ((float)allPawns[i].inventory.health * (float)healthPercentage / 100.0);
        }
    }

    return defenseValue;
}

CPULOGIC_SECTION
static Pawn *cpuLogic_weakestOtherFactionHomeBaseWithFlag(Pawn *pawn, Pawn *allPawns, int totalPawnCount) {
    int i;
    Pawn *weakestHomeBase = NULL;
    int weakestDefence = 9999;
    int indices[totalPawnCount];
    mathIsFun_shuffleIndices(indices, totalPawnCount);

    for (i = 0; i < totalPawnCount; i++) {
        int index = indices[i];
        if (!isInvalidCoordinate(allPawns[index].position) && allPawns[index].type == PAWNTYPE_BASE && allPawns[index].faction != pawn->faction && allPawns[index].inventory.carryingFlag) {
            int defenseValue = cpuLogic_defenseValueForPawn(&allPawns[index], allPawns, totalPawnCount);
            if (defenseValue < weakestDefence) {
                weakestDefence = defenseValue;
                weakestHomeBase = &allPawns[index];
            }
        }
    }
    return weakestHomeBase;
}

CPULOGIC_SECTION
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
CPULOGIC_SECTION
static Coordinate cpuLogic_safePosition(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Pawn *target, Boolean canGoToBase, Boolean cpuPlayersOnly) {
    int maxRange = GAMEMECHANICS_MAXTILEMOVERANGE;
    int dx, dy;
    int minDistance = 9999;
    int maxDamage = cpuLogic_pawnWithStolenFlag(pawn, allPawns, totalPawnCount, 0, true) != NULL ? 9999 : (random(0, 5) >= 4 || cpuPlayersOnly) ? (pawn->inventory.health * 0.5)
                                                                                                                                                : 10;
    Coordinate targetPosition, safePosition;
    int rangeIndicesCount = maxRange * 2 + 1;
    int indicesX[rangeIndicesCount];
    int indicesY[rangeIndicesCount];
    mathIsFun_shuffleIndices(indicesX, rangeIndicesCount);
    mathIsFun_shuffleIndices(indicesY, rangeIndicesCount);
    
    if (target == NULL || isInvalidCoordinate(target->position)) {
        return (Coordinate){-1, -1};
    }
    targetPosition = movement_closestTileToTargetInRange(pawn, target->position, allPawns, totalPawnCount, canGoToBase);
    safePosition = targetPosition;
    for (dx = -maxRange; dx <= maxRange; dx++) {
        int deltaX = indicesX[dx + maxRange] - maxRange;
        for (dy = -maxRange; dy <= maxRange; dy++) {
            int deltaY = indicesY[dy + maxRange] - maxRange;
            Coordinate candidateTile = {pawn->position.x + deltaX, pawn->position.y + deltaY};
            int possibleDamage = cpuLogic_damageAssementForTile(candidateTile, pawn, allPawns, totalPawnCount);
            if (movement_shipAtTarget(candidateTile, allPawns, totalPawnCount) != NULL) {
                continue;
            }
#ifdef DEBUG
            drawhelper_drawTextWithValue("", possibleDamage, viewport_convertedCoordinate(hexgrid_tileCenterPosition(candidateTile)));
#endif
            if (isPositionInBounds(candidateTile) && possibleDamage <= maxDamage && movement_distance(pawn->position, candidateTile) <= maxRange) {
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

CPULOGIC_SECTION
static Pawn *cpuLogic_weakestEnemyInRange(Pawn *pawn, Pawn *allPawns, int totalPawnCount, Boolean includeBases, Boolean unlimitedRange, int range) {
    int i;
    Pawn *weakestEnemy = NULL;
    int indices[totalPawnCount];
    mathIsFun_shuffleIndices(indices, totalPawnCount);

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

CPULOGIC_SECTION
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

CPULOGIC_SECTION
static CPUStrategyResult cpuLogic_defendBaseStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-30, 30), CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
    Pawn *homeBase = movement_homeBase(pawn->faction, allPawns, totalPawnCount);

    if (homeBase->inventory.carryingFlag == false) {  // flag was stolen! If enemy with flag is in range, attack, otherwise move to home base
        Pawn *enemyWithFlag = cpuLogic_pawnWithStolenFlag(pawn, allPawns, totalPawnCount, 0, true);
        strategyResult.score += 50;
        if (enemyWithFlag != NULL) {
            if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base or enemy with flag, whichever is closer
                Pawn *enemyHomeBase = movement_homeBase(enemyWithFlag->faction, allPawns, totalPawnCount);
                int distanceFromEnemy = movement_distance(enemyWithFlag->position, pawn->position);
                int distanceFromEnemyBase = movement_distance(enemyHomeBase->position, pawn->position);
                if (enemyHomeBase != NULL && distanceFromEnemyBase < distanceFromEnemy) {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyHomeBase;
                } else {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyWithFlag;
                } 
            }
        }  // No else case, this means the flag was succesfully brought to the enemy base
    } else {
        Pawn *enemyInRangeOfHomeBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, false, false, (float)GAMEMECHANICS_MAXTILEMOVERANGE);
        if (enemyInRangeOfHomeBase != NULL) {
            strategyResult.score += 80;
            if (!cpuLogic_attackIfInRange(pawn, enemyInRangeOfHomeBase, &strategyResult)) {  // Attack if we can, if not, move towards enemy
                Boolean farAway = (float)movement_distance(homeBase->position, pawn->position) > (float)GAMEMECHANICS_MAXTILEMOVERANGE * 2;
                if (!pawn->warped && farAway) {
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

CPULOGIC_SECTION
static CPUStrategyResult cpuLogic_captureTheFlagStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-30, 30), CPUACTION_NONE, NULL, (Coordinate){-1, -1}, true};
    if (pawn->inventory.carryingFlag) {  // carrying flag, move to home base
        Pawn *homeBase = movement_homeBase(pawn->faction, allPawns, totalPawnCount);
        strategyResult.CPUAction = CPUACTION_MOVE;
        strategyResult.target = homeBase;
        strategyResult.score += 150;
    } else {  // not carrying flag, try to capture it
        int distance;
        Pawn *enemyHomeBase = cpuLogic_weakestOtherFactionHomeBaseWithFlag(pawn, allPawns, totalPawnCount);
        if (enemyHomeBase == NULL) {
            strategyResult.score -= 120;
            strategyResult.allowMoveToBase = false;
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
                strategyResult.allowMoveToBase = false;
                return strategyResult;
            }
        } else {  // if not, move towards enemy home base
            strategyResult.CPUAction = CPUACTION_MOVE;
            strategyResult.target = enemyHomeBase;
        }
    }

    return strategyResult;
}

CPULOGIC_SECTION
static CPUStrategyResult cpuLogic_attackStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int factionValue) {
    CPUStrategyResult strategyResult = {factionValue + random(-40, 40), CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};

    Pawn *enemyWithFlag = cpuLogic_pawnWithStolenFlag(pawn, allPawns, totalPawnCount, 0, true);
    if (enemyWithFlag != NULL) {
        strategyResult.score += 50;
        if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {  // Attack if we can, if not, move to enemy home base
            Pawn *enemyHomeBase = movement_homeBase(enemyWithFlag->faction, allPawns, totalPawnCount);
            if (enemyHomeBase != NULL) {
                if (!cpuLogic_attackIfInRange(pawn, enemyWithFlag, &strategyResult)) {
                    strategyResult.CPUAction = CPUACTION_MOVE;
                    strategyResult.target = enemyHomeBase;
                }
            }
        }
    } else {
        Pawn *nearestEnemyShipOrBase = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, true, 1);
        if (nearestEnemyShipOrBase != NULL) {
            if (!cpuLogic_attackIfInRange(pawn, nearestEnemyShipOrBase, &strategyResult)) {  // Attack if we can, if not, move to enemy
                strategyResult.CPUAction = CPUACTION_MOVE;
                strategyResult.target = nearestEnemyShipOrBase;
            }
        }
    }

    return strategyResult;
}

CPULOGIC_SECTION
static CPUStrategyResult cpuLogic_provideBackupStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, CPUFactionProfile factionProfile) {
    // iterate through all other ship pawns of the same faction, disregard ships that are close to home base
    // if one of them is in danger of being attacked, move towards potential enemy
    int i;
    int minimalDefenseValue = 9999;
    Pawn *homeBase = movement_homeBase(pawn->faction, allPawns, totalPawnCount);
    Pawn *pawnInDanger = cpuLogic_pawnWithStolenFlag(pawn, allPawns, totalPawnCount, 0, false);

    if (pawnInDanger == NULL) {
        for (i = 0; i < totalPawnCount; i++) {
            if (!isInvalidCoordinate(allPawns[i].position) && pawn != &allPawns[i] && allPawns[i].type == PAWNTYPE_SHIP && allPawns[i].faction == pawn->faction && allPawns[i].type == PAWNTYPE_SHIP && (float)movement_distance(homeBase->position, allPawns[i].position) >= (float)GAMEMECHANICS_MAXTILEMOVERANGE * 1.5) {
                int defenseValue = cpuLogic_defenseValueForPawn(&allPawns[i], allPawns, totalPawnCount);
                if (defenseValue < minimalDefenseValue) {
                    minimalDefenseValue = defenseValue;
                    pawnInDanger = &allPawns[i];
                }
            }
        }
    }
    if (pawnInDanger != NULL && minimalDefenseValue >= GAMEMECHANICS_MAXSHIPHEALTH) {
        CPUStrategyResult strategyResult = {(factionProfile.captureFlagPriority + factionProfile.defendBasePriority) / 2 + random(-20, 60), CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
        Pawn *enemyInRange = cpuLogic_weakestEnemyInRange(pawnInDanger, allPawns, totalPawnCount, false, false, (float)GAMEMECHANICS_MAXTILETORPEDORANGE * 1.5);
        if (enemyInRange != NULL && !cpuLogic_attackIfInRange(pawn, enemyInRange, &strategyResult)) {  // Attack if we can, if not, move to enemy
            strategyResult.CPUAction = CPUACTION_MOVE;
            strategyResult.target = enemyInRange;
        }
        return strategyResult;
    } else {
        return (CPUStrategyResult){0, CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
    }
}

CPULOGIC_SECTION
static CPUStrategyResult cpuLogic_baseStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int currentTurn, CPUFactionProfile factionProfile) {
    Pawn *enemyInShortRange;

    if (pawn_baseTurnsLeft(currentTurn, pawn->inventory.baseActionLastActionTurn, pawn->inventory.lastBaseAction) > 0) {
        return (CPUStrategyResult){0, CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
    }

    enemyInShortRange = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, false, (float)GAMEMECHANICS_SHOCKWAVERANGE * 0.7);
    if (enemyInShortRange != NULL && cpuLogic_pawnWithStolenFlag(pawn, allPawns, totalPawnCount, GAMEMECHANICS_SHOCKWAVERANGE, true) == NULL) {
        return (CPUStrategyResult){100, CPUACTION_BASE_SHOCKWAVE, NULL, (Coordinate){-1, -1}, false};
    } else {
        Pawn *enemyInLongRange = cpuLogic_weakestEnemyInRange(pawn, allPawns, totalPawnCount, true, false, (float)GAMEMECHANICS_MAXTILEMOVERANGE * 1.2);
        if (enemyInLongRange != NULL && factionProfile.attackPriority < factionProfile.defendBasePriority) {
            return (CPUStrategyResult){0, CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
        } else {
            return (CPUStrategyResult){100, CPUACTION_BASE_BUILDSHIP, NULL, (Coordinate){-1, -1}, false};
        }
    }
}

CPULOGIC_SECTION
CPUStrategyResult cpuLogic_getStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, int currentTurn, CPUFactionProfile factionProfile, Boolean cpuPlayersOnly) {
    int i;
    CPUStrategyResult bestStrategy;
    CPUStrategyResult strategyResult[4];
#ifdef DEBUG
    CPUStrategy selectedStrategy = 0;
#endif
    if (isInvalidCoordinate(pawn->position)) {
        return (CPUStrategyResult){100, CPUACTION_NONE, NULL, (Coordinate){-1, -1}, false};
    }
    if (pawn->type == PAWNTYPE_BASE) {
        return cpuLogic_baseStrategy(pawn, allPawns, totalPawnCount, currentTurn, factionProfile);
    }

    strategyResult[CPUSTRATEGY_DEFENDBASE] = cpuLogic_defendBaseStrategy(pawn, allPawns, totalPawnCount, factionProfile.defendBasePriority);
    strategyResult[CPUSTRATEGY_CAPTUREFLAG] = cpuLogic_captureTheFlagStrategy(pawn, allPawns, totalPawnCount, factionProfile.captureFlagPriority);
    strategyResult[CPUSTRATEGY_ATTACK] = cpuLogic_attackStrategy(pawn, allPawns, totalPawnCount, factionProfile.attackPriority);
    strategyResult[CPUSTRATEGY_PROVIDEBACKUP] = cpuLogic_provideBackupStrategy(pawn, allPawns, totalPawnCount, factionProfile);

    bestStrategy = strategyResult[0];
    for (i = 1; i < 4; i++) {
        if (strategyResult[i].CPUAction == CPUACTION_NONE) {
            strategyResult[i].score = -999;
        }
        if (strategyResult[i].score > bestStrategy.score) {
            bestStrategy = strategyResult[i];
#ifdef DEBUG
            selectedStrategy = i;
#endif
        }
    }
    // bestStrategy = strategyResult[CPUSTRATEGY_DEFENDBASE];
    bestStrategy.targetPosition = cpuLogic_safePosition(pawn, allPawns, totalPawnCount, bestStrategy.target, bestStrategy.allowMoveToBase, cpuPlayersOnly);

#ifdef DEBUG
    /*c
    drawhelper_drawTextWithValue("CTF:", strategyResult[CPUSTRATEGY_CAPTUREFLAG].score, (Coordinate){0, 10});
    drawhelper_drawTextWithValue("ATTACK:", strategyResult[CPUSTRATEGY_ATTACK].score, (Coordinate){0, 20});*/
    switch (selectedStrategy) {
        case CPUSTRATEGY_DEFENDBASE:
            drawhelper_drawText("STRAT: DEFBASE", (Coordinate){0, 0});
            break;
        case CPUSTRATEGY_CAPTUREFLAG:
            drawhelper_drawText("STRAT: CTF", (Coordinate){0, 0});
            break;
        case CPUSTRATEGY_ATTACK:
            drawhelper_drawText("STRAT: ATT", (Coordinate){0, 0});
            break;
        case CPUSTRATEGY_PROVIDEBACKUP:
            drawhelper_drawText("STRAT: BACKUP", (Coordinate){0, 0});
            break;
    }
#endif

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
        case CPUACTION_BASE_SHOCKWAVE:
            drawhelper_drawText("SHOCK", (Coordinate){0, 30});
            break;
        case CPUACTION_BASE_BUILDSHIP:
            drawhelper_drawText("BUILD", (Coordinate){0, 30});
            break;
    }
    sleep(1000);
#endif
    return bestStrategy;
}
