#include "level.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../graphicResources.h"
#include "mathIsFun.h"
#include "movement.h"
#include "pawn.h"

LEVEL_SECTION
static void level_scoreText(char *fixedText, DmResID textResource, int value) {
    MemHandle resourceHandle = DmGetResource(strRsc, textResource);
    char *resourceText = (char *)MemHandleLock(resourceHandle);
    StrIToA(fixedText, value);
    StrCat(fixedText, " ");
    StrCat(fixedText, resourceText);
    MemHandleUnlock(resourceHandle);
    DmReleaseResource(resourceHandle);
}

LEVEL_SECTION
static void level_addPawns(Pawn *newPawns, int additionalPawnCount, Level *level) {
    int i;
    Pawn *updatedPawns = MemPtrNew(sizeof(Pawn) * (level->pawnCount + additionalPawnCount));
    if (level->pawns != NULL) {
        MemSet(updatedPawns, sizeof(Pawn) * (level->pawnCount + additionalPawnCount), 0);
        MemMove(updatedPawns, level->pawns, sizeof(Pawn) * level->pawnCount);
        MemPtrFree(level->pawns);
    }

    level->pawns = updatedPawns;
    for (i = 0; i < additionalPawnCount; i++) {
        level->pawns[level->pawnCount] = newPawns[i];
        level->pawnCount++;
    }
}

LEVEL_SECTION
UInt8 level_factionCount(NewGameConfig config) {
    int i, activePlayers = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        if (config.playerConfig[i].active) {
            activePlayers++;
        }
    }
    return activePlayers;
}

LEVEL_SECTION
Level level_startLevel() {
    // Start screen options:
    // - New
    // - Rank
    // - About

    Level level;
    Pawn newPawns[4];
    level.actionTileCount = 0;
    level.actionTiles = NULL;
    level.pawnCount = 0;
    level.pawns = NULL;
    MemSet(newPawns, sizeof(Pawn) * 4, 0);
    newPawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, BASEACTION_NONE, false}, 4, 3, false, false};
    newPawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, BASEACTION_NONE, false}, 4, 0, false, false};
    newPawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, BASEACTION_NONE, false}, 4, 1, false, false};
    newPawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, BASEACTION_NONE, false}, 4, 2, false, false};
    level_addPawns(newPawns, 4, &level);

    level.gridTexts = MemPtrNew(sizeof(GridText) * 3);
    MemSet(level.gridTexts, sizeof(GridText) * 3, 0);
    level.gridTextCount = 3;
    level.gridTexts[0] = (GridText){(Coordinate){1, 2}, (Coordinate){0, 0}, STRING_NEW, NULL, false, false};
    level.gridTexts[1] = (GridText){(Coordinate){1, 4}, (Coordinate){0, 0}, STRING_RANK, NULL, false, false};
    level.gridTexts[2] = (GridText){(Coordinate){1, 6}, (Coordinate){0, 0}, STRING_ABOUT, NULL, false, false};

    return level;
}

LEVEL_SECTION
void level_applyNewGameConfig(NewGameConfig config, Level *level) {
    int i;
    int activePlayers = level_factionCount(config);

    for (i = 0; i < level->actionTileCount; i++) {
        switch (level->actionTiles[i].identifier) {
            case ACTIONTILEIDENTIFIER_HUMANPLAYER:
                level->actionTiles[i].selected = config.playerConfig[level->actionTiles[i].tag].isHuman;
                level->actionTiles[i].hidden = level->actionTiles[i].tag > activePlayers - 1;
                break;
            case ACTIONTILEIDENTIFIER_CPUPLAYER:
                level->actionTiles[i].selected = !config.playerConfig[level->actionTiles[i].tag].isHuman;
                level->actionTiles[i].hidden = level->actionTiles[i].tag > activePlayers - 1;
                break;
            case ACTIONTILEIDENTIFIER_LAUNCHGAME:
            case ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS:
            case ACTIONTILEIDENTIFIER_ENDGAME:
                break;
            case ACTIONTILEIDENTIFIER_TWOPLAYERS:
            case ACTIONTILEIDENTIFIER_THREEPLAYERS:
            case ACTIONTILEIDENTIFIER_FOURPLAYERS:
                level->actionTiles[i].selected = level->actionTiles[i].tag == activePlayers;
                break;
        }
    }
}

LEVEL_SECTION
NewGameConfig level_getNewGameConfig(Level *level, NewGameConfig oldConfig) {
    NewGameConfig config;
    int i, activePlayers;
    for (i = 0; i < level->actionTileCount; i++) {
        if (!level->actionTiles[i].selected) {
            continue;
        }
        switch (level->actionTiles[i].identifier) {
            case ACTIONTILEIDENTIFIER_HUMANPLAYER:
                config.playerConfig[level->actionTiles[i].tag].isHuman = true;
                break;
            case ACTIONTILEIDENTIFIER_CPUPLAYER:
                config.playerConfig[level->actionTiles[i].tag].isHuman = false;
                break;
            case ACTIONTILEIDENTIFIER_LAUNCHGAME:
            case ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS:
            case ACTIONTILEIDENTIFIER_ENDGAME:
                break;
            case ACTIONTILEIDENTIFIER_TWOPLAYERS:
            case ACTIONTILEIDENTIFIER_THREEPLAYERS:
            case ACTIONTILEIDENTIFIER_FOURPLAYERS:
                activePlayers = level->actionTiles[i].tag;
                break;
        }
    }

    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        config.playerConfig[i].active = i < activePlayers;
    }

    config.placementStrategy = oldConfig.placementStrategy;
    config.shipCount = oldConfig.shipCount;
    return config;
}

LEVEL_SECTION
Boolean level_movesLeftForFaction(int faction, int currentTurn, Level *level) {
    int i;
    for (i = 0; i < level->pawnCount; i++) {
        Boolean turnComplete;
        switch (level->pawns[i].type) {
            case PAWNTYPE_SHIP:
                turnComplete = level->pawns[i].turnComplete;
                break;
            case PAWNTYPE_BASE:
                turnComplete = level->pawns[i].turnComplete || pawn_baseTurnsLeft(currentTurn, level->pawns[i].inventory.baseActionLastActionTurn, level->pawns[i].inventory.lastBaseAction) > 0;
                break;
        }
        if (level->pawns[i].faction == faction && !turnComplete) {
            return true;
        }
    }
    return false;
}

LEVEL_SECTION
Pawn *level_nextPawn(Pawn *currentPawn, Boolean allPawns, Boolean onlyWithAvailableActions, int factionTurn, int currentTurn, Level *level) {
    Pawn *firstPawn = NULL;
    int i;
    int startMatching = false;
    currentPawn = currentPawn->faction == factionTurn ? currentPawn : NULL;
    for (i = 0; i < level->pawnCount; i++) {
        if (level->pawns[i].faction == factionTurn && !isInvalidCoordinate(level->pawns[i].position)) {
            if (onlyWithAvailableActions && level->pawns[i].turnComplete) {
                continue;
            }
            if (!allPawns && level->pawns[i].type == PAWNTYPE_BASE && pawn_baseTurnsLeft(currentTurn, level->pawns[i].inventory.baseActionLastActionTurn, level->pawns[i].inventory.lastBaseAction) > 0) {
                continue;
            }
            if (firstPawn == NULL) {
                firstPawn = &level->pawns[i];
            }
            if (startMatching) {
                return &level->pawns[i];
            }
            if (currentPawn == &level->pawns[i]) {
                startMatching = true;
            }
        }
    }
    return firstPawn;
}

LEVEL_SECTION
void level_reorderPawnsByDistance(Level *level) {
    int i, j;
    if (level->pawnCount < 2) {
        return;
    }
    for (i = 0; i < level->pawnCount - 1; i++) {
        for (j = 0; j < level->pawnCount - i - 1; j++) {
            int distA = abs(level->pawns[j].position.x) + abs(level->pawns[j].position.y);
            int distB = abs(level->pawns[j + 1].position.x) + abs(level->pawns[j + 1].position.y);
            if (distA > distB) {
                Pawn temp = level->pawns[j];
                level->pawns[j] = level->pawns[j + 1];
                level->pawns[j + 1] = temp;
            }
        }
    }
}

LEVEL_SECTION
void level_removePawnAtIndex(int index, Level *level) {
    int i;
    for (i = index; i < level->pawnCount - 1; i++) {
        level->pawns[i] = level->pawns[i + 1];
    }
    level->pawnCount--;
    MemPtrResize(level->pawns, sizeof(Pawn) * level->pawnCount);
}

LEVEL_SECTION
static int level_removePawnsOfFaction(int factionIndex, Level *level) {
    int i;
    int removalCount = 0;
    for (i = 0; i < level->pawnCount - 1; i++) {
        if (level->pawns[i].faction == factionIndex) {
            level_removePawnAtIndex(i, level);
            i--;
            removalCount++;
        }
    }
    return removalCount;
}

LEVEL_SECTION
void level_removePawn(Pawn *pawn, Level *level) {
    int i;
    for (i = 0; i < level->pawnCount; i++) {
        if (isEqualCoordinate(pawn->position, level->pawns[i].position)) {  // if (pawn == &level->pawns[i]) {
            level_removePawnAtIndex(i, level);
            return;
        }
    }
}

LEVEL_SECTION
static void level_removePawnsBelowCoordinates(Coordinate coord, Level *level, Boolean inverse) {
    int i;
    for (i = 0; i < level->pawnCount; i++) {
        Boolean affected = level->pawns[i].position.x < coord.x && level->pawns[i].position.y < coord.y;
        if (inverse) {
            affected = !affected;
        }
        if (affected) {
            level_removePawnAtIndex(i, level);
            level_removePawnsBelowCoordinates(coord, level, inverse);
            return;
        }
    }
}

LEVEL_SECTION
void level_addScorePawns(Level *level, int faction) {
    Pawn *newPawns;
    LevelScore score = level->scores[faction];
    Score generalScore = scoring_scoreFromLevelScores(level->scores, faction);
    int totalDestroyed = scoring_totalDestroyedShips(score) + scoring_totalDestroyedBases(score);
    int totalCaptured = scoring_totalCapturedShips(score);
    int i, factionIndex, pawnCount = 0;
    level_removePawnsBelowCoordinates((Coordinate){9, 9}, level, false);
    if (level->gridTexts != NULL) {
        MemPtrFree(level->gridTexts);
        level->gridTexts = NULL;
    }
    level->gridTextCount = 6;
    level->gridTexts = MemPtrNew(sizeof(GridText) * (level->gridTextCount));
    level->gridTexts[0] = (GridText){(Coordinate){0, 1}, (Coordinate){HEXTILE_SIZE / 4, 4}, STRING_DESTROYED, "", false, true, false};
    level->gridTexts[1] = (GridText){(Coordinate){0, 3}, (Coordinate){HEXTILE_SIZE / 4, 4}, STRING_CAPTURED, "", false, true, false};
    level->gridTexts[2] = (GridText){(Coordinate){0, 2}, (Coordinate){HEXTILE_SIZE / 2, 4}, 0, "", false, true, false};
    StrIToA(level->gridTexts[2].fixedText, totalDestroyed);
    level->gridTexts[3] = (GridText){(Coordinate){0, 4}, (Coordinate){HEXTILE_SIZE / 2, 4}, 0, "", false, true, false};
    StrIToA(level->gridTexts[3].fixedText, totalCaptured);
    level->gridTexts[4] = (GridText){(Coordinate){0, 5}, (Coordinate){HEXTILE_SIZE / 2, 10}, 0, "", false, true, false};
    level_scoreText(level->gridTexts[4].fixedText, STRING_FLAGSSTOLEN, score.flagsStolen);
    level->gridTexts[5] = (GridText){(Coordinate){0, 6}, (Coordinate){HEXTILE_SIZE / 2, 10}, 0, "", false, true, false};
    level_scoreText(level->gridTexts[5].fixedText, STRING_SHIPSLOST, generalScore.shipsLost);

    if (level->actionTiles != NULL) {
        MemPtrFree(level->actionTiles);
        level->actionTiles = NULL;
    }
    level->actionTiles = MemPtrNew(sizeof(ActionTile));
    level->actionTileCount = 1;
    level->actionTiles[0] = (ActionTile){(Coordinate){6, 7}, true, ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS, false, 0};

    // add destroyed ships
    if (totalDestroyed > 0) {
        newPawns = MemPtrNew(sizeof(Pawn) * totalDestroyed);
        pawnCount = 0;
        for (factionIndex = 0; factionIndex < GAMEMECHANICS_MAXPLAYERCOUNT; factionIndex++) {
            if (score.basesDestroyed[factionIndex]) {
                newPawns[pawnCount] = (Pawn){PAWNTYPE_BASE, (Coordinate){1 + pawnCount, 2}, (Inventory){1, 0, 0, 0, BASEACTION_NONE, false}, 4, factionIndex, false, false};
                pawnCount++;
            }
            for (i = 0; i < score.shipsDestroyed[factionIndex]; i++) {
                newPawns[pawnCount] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1 + pawnCount, 2}, (Inventory){1, 0, 0, 0, BASEACTION_NONE, false}, 4, factionIndex, false, false};
                pawnCount++;
            }
        }
        level_addPawns(newPawns, pawnCount, level);
        MemPtrFree(newPawns);
    }

    // add captured ships
    if (totalCaptured > 0) {
        newPawns = MemPtrNew(sizeof(Pawn) * totalCaptured);
        pawnCount = 0;
        for (factionIndex = 0; factionIndex < GAMEMECHANICS_MAXPLAYERCOUNT; factionIndex++) {
            for (i = 0; i < score.shipsCaptured[factionIndex]; i++) {
                newPawns[pawnCount] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1 + pawnCount, 4}, (Inventory){1, 0, 0, 0, BASEACTION_NONE, false}, 4, factionIndex, false, false};
                pawnCount++;
            }
        }
        level_addPawns(newPawns, pawnCount, level);
        MemPtrFree(newPawns);
    }
}

LEVEL_SECTION
void level_addRank(Level *level, Score score) {
    int additionalGridTexts = 6;
    GridText *updatedGridTexts = MemPtrNew(sizeof(GridText) * (level->gridTextCount + additionalGridTexts));
    level_removePawnsBelowCoordinates((Coordinate){6, 6}, level, true);
    MemSet(updatedGridTexts, sizeof(GridText) * (level->gridTextCount + additionalGridTexts), 0);
    MemMove(updatedGridTexts, level->gridTexts, sizeof(GridText) * level->gridTextCount);
    MemPtrFree(level->gridTexts);
    level->gridTexts = updatedGridTexts;

    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 2}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_UNTILNEXTRANK, scoring_scoreNeededUntilNextRank(score) * GAMEMECHANICS_SCOREBOOST);
    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 3}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_SHIPSDESTROYED, score.shipsDestroyed);
    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 4}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_SHIPSCAPTURED, score.shipsCaptured);
    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 5}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_FLAGSSTOLEN, score.flagsStolen);
    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 6}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_FLAGSCAPTURED, score.flagsCaptured);
    level->gridTexts[level->gridTextCount++] = (GridText){(Coordinate){8, 7}, (Coordinate){0, 0}, 0, "", false, true};
    level_scoreText(level->gridTexts[level->gridTextCount - 1].fixedText, STRING_SHIPSLOST, score.shipsLost);

    if (level->actionTiles != NULL) {
        MemPtrFree(level->actionTiles);
        level->actionTiles = NULL;
    }
    level->actionTiles = MemPtrNew(sizeof(ActionTile));
    level->actionTileCount = 1;
    level->actionTiles[0] = (ActionTile){(Coordinate){13, 7}, true, ACTIONTILEIDENTIFIER_ENDGAME, false, 0};
}

LEVEL_SECTION
void level_addPlayerConfigPawns(Level *level, NewGameConfig newGameConfig) {
    int i;
    int index;
    int additionalPawnCount = 4;
    int additionalGridTexts = 1;
    GridText *updatedGridTexts;
    Pawn *updatedPawns = MemPtrNew(sizeof(Pawn) * (additionalPawnCount));
    MemSet(updatedPawns, sizeof(Pawn) * (additionalPawnCount), 0);
    updatedPawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, 4, 0, false, false};
    updatedPawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, 4, 1, false, false};
    updatedPawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, 4, 2, false, false};
    updatedPawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, 4, 3, false, false};
    level_addPawns(updatedPawns, 4, level);
    MemPtrFree(updatedPawns);

    level->actionTileCount = additionalPawnCount * 2 + 4;
    level->actionTiles = MemPtrNew(sizeof(ActionTile) * level->actionTileCount);
    MemSet(level->actionTiles, sizeof(ActionTile) * level->actionTileCount, 0);
    index = 0;
    for (i = additionalPawnCount; i < level->pawnCount; i++) {
        level->actionTiles[index] = (ActionTile){(Coordinate){level->pawns[i].position.x + 1, level->pawns[i].position.y}, i == additionalPawnCount, ACTIONTILEIDENTIFIER_HUMANPLAYER, false, i - additionalPawnCount};    // HUMAN PLAYER
        level->actionTiles[index + 1] = (ActionTile){(Coordinate){level->pawns[i].position.x + 2, level->pawns[i].position.y}, i != additionalPawnCount, ACTIONTILEIDENTIFIER_CPUPLAYER, false, i - additionalPawnCount};  // PALM PLAYER
        index = index + 2;
    }

    index = level->actionTileCount - 4;
    level->actionTiles[index] = (ActionTile){(Coordinate){8, 7}, false, ACTIONTILEIDENTIFIER_TWOPLAYERS, false, 2};        // 2 player config
    level->actionTiles[index + 1] = (ActionTile){(Coordinate){9, 7}, true, ACTIONTILEIDENTIFIER_THREEPLAYERS, false, 3};   // 3 player config
    level->actionTiles[index + 2] = (ActionTile){(Coordinate){10, 7}, false, ACTIONTILEIDENTIFIER_FOURPLAYERS, false, 4};  // 4 player config

    level->actionTiles[index + 3] = (ActionTile){(Coordinate){13, 7}, true, ACTIONTILEIDENTIFIER_LAUNCHGAME, false, 0};  // Start the game

    updatedGridTexts = MemPtrNew(sizeof(GridText) * (level->gridTextCount + additionalGridTexts));
    MemSet(updatedGridTexts, sizeof(GridText) * (level->gridTextCount + additionalGridTexts), 0);
    MemMove(updatedGridTexts, level->gridTexts, sizeof(GridText) * level->gridTextCount);
    MemPtrFree(level->gridTexts);
    level->gridTexts = updatedGridTexts;

    level->gridTexts[level->gridTextCount] = (GridText){(Coordinate){8, 6}, (Coordinate){0, 0}, STRING_PLAYERS, "", false, true};
    level->gridTextCount = level->gridTextCount + 1;

    level_applyNewGameConfig(newGameConfig, level);
}

LEVEL_SECTION
void level_addPawn(Pawn pawn, Level *level) {
    level_addPawns(&pawn, 1, level);
}

LEVEL_SECTION
static void level_applyPlacementCorners(Level *level, NewGameConfig config) {
    int i, j;
    int indices[GAMEMECHANICS_MAXPLAYERCOUNT];
    int pawnIndex = 0;
    Coordinate baseCoordinates[] = {
        (Coordinate){1, 1}, (Coordinate){HEXGRID_COLS - 2, HEXGRID_ROWS - 2}, (Coordinate){1, HEXGRID_ROWS - 2}, (Coordinate){HEXGRID_COLS - 2, 1}};

    mathIsFun_shuffleIndices(indices, GAMEMECHANICS_MAXPLAYERCOUNT);

    // Set the bases
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        int faction = indices[i];
        Coordinate baseCoordinate = baseCoordinates[i];
        Pawn *basePawn;
        if (!config.playerConfig[faction].active) {
            continue;
        }
        level->pawns[pawnIndex] = (Pawn){PAWNTYPE_BASE, baseCoordinate, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, faction, 0, 0, BASEACTION_NONE, true}, 0, faction, false, false};
        basePawn = &level->pawns[pawnIndex];
        pawnIndex++;
        // Add ships around the bases
        for (j = 0; j < config.shipCount; j++) {
            Coordinate shipCoordinate = movement_closestTileToTargetInRange(basePawn, basePawn->position, level->pawns, pawnIndex, false);
            level->pawns[pawnIndex] = (Pawn){PAWNTYPE_SHIP, shipCoordinate, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, (faction + faction * TimGetTicks()) % GFX_FRAMECOUNT_SHIPA, faction, false, false};
            pawnIndex++;
        }
    }
}

LEVEL_SECTION
static void level_removeHumanPawnsAndRecenter(Level *level, NewGameConfig config) {
    int i, j;
    int indices[GAMEMECHANICS_MAXPLAYERCOUNT];
    int pawnIndex = 0;
    mathIsFun_shuffleIndices(indices, GAMEMECHANICS_MAXPLAYERCOUNT);
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        int faction = indices[i];
        Coordinate baseCoordinate = (Coordinate){HEXGRID_COLS / 2, HEXGRID_ROWS / 2};
        int extraPawnCount = config.shipCount + 1 + 1;
        Pawn pawns[extraPawnCount];
        Pawn *basePawn;
        if (!config.playerConfig[faction].isHuman) {
            continue;
        }
        level_removePawnsOfFaction(faction, level);

        pawns[pawnIndex] = (Pawn){PAWNTYPE_BASE, baseCoordinate, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, faction, 0, 0, BASEACTION_NONE, true}, 0, faction, false, false};
        basePawn = &pawns[pawnIndex];
        pawnIndex++;
        // Add ships around the bases
        for (j = 0; j < config.shipCount + 1; j++) {
            Coordinate shipCoordinate = movement_closestTileToTargetInRange(basePawn, basePawn->position, pawns, pawnIndex, false);
            pawns[pawnIndex] = (Pawn){PAWNTYPE_SHIP, shipCoordinate, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, (faction + faction * TimGetTicks()) % GFX_FRAMECOUNT_SHIPA, faction, false, false};
            pawnIndex++;
        }
        level_addPawns(pawns, pawnIndex, level);
        return;
    }
}

LEVEL_SECTION
Level level_create(NewGameConfig config) {
    Level level;
    int factionCount = level_factionCount(config);

    level.gridTexts = NULL;
    level.gridTextCount = 0;

    level.actionTiles = NULL;
    level.actionTileCount = 0;

    level.pawnCount = factionCount + factionCount * config.shipCount;  // Bases + ships
    level.pawns = MemPtrNew(sizeof(Pawn) * level.pawnCount);
    MemSet(level.pawns, sizeof(Pawn) * level.pawnCount, 0);

    MemSet(level.scores, sizeof(LevelScore) * GAMEMECHANICS_MAXPLAYERCOUNT, 0);

    switch (config.placementStrategy) {
        case PLAYERPLACEMENTSTRATEGY_CORNERS:
            level_applyPlacementCorners(&level, config);
            break;
        case PLAYERPLACEMENTSTRATEGY_CENTERDEFENSE:
            level_applyPlacementCorners(&level, config);
            level_removeHumanPawnsAndRecenter(&level, config);
            break;
    }

    return level;
}

LEVEL_SECTION
void level_destroy(Level *level) {
    if (level == NULL) {
        return;
    }
    if (level->pawns != NULL) {
        level->pawnCount = 0;
        MemPtrFree(level->pawns);
        level->pawns = NULL;
    }
    if (level->gridTexts != NULL) {
        level->gridTextCount = 0;
        MemPtrFree(level->gridTexts);
        level->gridTexts = NULL;
    }
    if (level->actionTiles != NULL) {
        level->actionTileCount = 0;
        MemPtrFree(level->actionTiles);
        level->actionTiles = NULL;
    }
}

LEVEL_SECTION
NewGameConfig level_defaultNewGameConfig(int rank) {
    NewGameConfig config;
    int i;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        config.playerConfig[i].active = true;
        config.playerConfig[i].isHuman = i == 0;
    }
    config.playerConfig[GAMEMECHANICS_MAXPLAYERCOUNT - 1].active = rank > 1;
    config.playerConfig[GAMEMECHANICS_MAXPLAYERCOUNT - 2].active = rank > 0;
    if (rank < 1) {
        config.placementStrategy = PLAYERPLACEMENTSTRATEGY_CORNERS;
    } else {
        config.placementStrategy = random(0, 1);
    }
    config.shipCount = 2;
    return config;
}

LEVEL_SECTION
Pawn *level_pawnAtTile(Coordinate tileCoordinate, Level *level) {
    int i;
    for (i = 0; i < level->pawnCount; i++) {
        if (isEqualCoordinate(level->pawns[i].position, tileCoordinate) && !isInvalidCoordinate(level->pawns[i].position)) {
            return &level->pawns[i];
        }
    }
    return NULL;
}

LEVEL_SECTION
void level_returnFlagFromPawnToOriginalBase(Pawn *pawn, Level *level) {
    Pawn *flagHomeBase;
    if (!pawn->inventory.carryingFlag) {
        return;
    }
    flagHomeBase = movement_homeBase(pawn->inventory.flagOfFaction, level->pawns, level->pawnCount);
    if (flagHomeBase != NULL) {
        flagHomeBase->inventory.carryingFlag = true;
    }
    pawn->inventory.carryingFlag = false;
}
