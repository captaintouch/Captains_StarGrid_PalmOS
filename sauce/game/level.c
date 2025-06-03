#include "level.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../graphicResources.h"
#include "mathIsFun.h"
#include "movement.h"

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
    Pawn *newPawns;
    level.actionTileCount = 0;
    level.actionTiles = NULL;
    level.pawnCount = 0;
    level.pawns = NULL;
    newPawns = MemPtrNew(sizeof(Pawn) * 4);
    MemSet(newPawns, sizeof(Pawn) * 4, 0);
    newPawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 3, false, false};
    newPawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 0, false, false};
    newPawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 1, false, false};
    newPawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 2, false, false};
    level_addPawns(newPawns, 4, &level);
    MemPtrFree(newPawns);

    level.gridTexts = MemPtrNew(sizeof(GridText) * 3);
    MemSet(level.gridTexts, sizeof(GridText) * 3, 0);
    level.gridTextCount = 3;
    level.gridTexts[0] = (GridText){(Coordinate){1, 2}, STRING_NEW, false, false};
    level.gridTexts[1] = (GridText){(Coordinate){1, 4}, STRING_RANK, false, false};
    level.gridTexts[2] = (GridText){(Coordinate){1, 6}, STRING_ABOUT, false, false};

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
void level_addScorePawns(Level *level) {
    // TODO: Remove all pawns that are on Page 1 and Page 2
    if (level->gridTexts != NULL) {
        MemPtrFree(level->gridTexts);
        level->gridTexts = NULL;
    }
    level->gridTexts = MemPtrNew(sizeof(GridText) * (2));
    MemSet(level->gridTexts, sizeof(GridText) * (2), 0);
    level->gridTexts[0] = (GridText){(Coordinate){1, 1}, STRING_DESTROYED, false, true, false};
    level->gridTexts[1] = (GridText){(Coordinate){1, 3}, STRING_CAPTURED, false, true, false};
    level->gridTextCount = 2;
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
    updatedPawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, false}, 4, 0, false, false};
    updatedPawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, false}, 4, 1, false, false};
    updatedPawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, false}, 4, 2, false, false};
    updatedPawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, false}, 4, 3, false, false};
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
    level->actionTiles[index + 1] = (ActionTile){(Coordinate){9, 7}, false, ACTIONTILEIDENTIFIER_THREEPLAYERS, false, 3};  // 3 player config
    level->actionTiles[index + 2] = (ActionTile){(Coordinate){10, 7}, true, ACTIONTILEIDENTIFIER_FOURPLAYERS, false, 4};   // 4 player config

    level->actionTiles[index + 3] = (ActionTile){(Coordinate){13, 7}, true, ACTIONTILEIDENTIFIER_LAUNCHGAME, false, 0};  // Start the game

    updatedGridTexts = MemPtrNew(sizeof(GridText) * (level->gridTextCount + additionalGridTexts));
    MemSet(updatedGridTexts, sizeof(GridText) * (level->gridTextCount + additionalGridTexts), 0);
    MemMove(updatedGridTexts, level->gridTexts, sizeof(GridText) * level->gridTextCount);
    MemPtrFree(level->gridTexts);
    level->gridTexts = updatedGridTexts;

    level->gridTexts[level->gridTextCount] = (GridText){(Coordinate){8, 6}, STRING_PLAYERS, false, true};
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
        (Coordinate){1, 1}, (Coordinate){HEXGRID_COLS - 2, HEXGRID_ROWS - 2}, (Coordinate){1, HEXGRID_ROWS - 2}, (Coordinate){HEXGRID_COLS - 2, 1}
    };

    mathIsFun_shuffleIndices(indices, GAMEMECHANICS_MAXPLAYERCOUNT);

    // Set the bases
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        int faction = indices[i];
        Coordinate baseCoordinate = baseCoordinates[i];
        Pawn *basePawn;
        if (!config.playerConfig[faction].active) {
            continue;
        }
        level->pawns[pawnIndex] = (Pawn){PAWNTYPE_BASE, baseCoordinate, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, faction, 0, 0, true}, 0, faction, false, false};
        basePawn = &level->pawns[pawnIndex];
        pawnIndex++;
        // Add ships around the bases
        for (j = 0; j < config.shipCount; j++) {
            Coordinate shipCoordinate = movement_closestTileToTargetInRange(basePawn, basePawn->position, level->pawns, pawnIndex, false);
            level->pawns[pawnIndex] = (Pawn){PAWNTYPE_SHIP, shipCoordinate, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, false}, (i + i * TimGetTicks()) % GFX_FRAMECOUNT_SHIPA , faction, false, false};
            pawnIndex++;
        }
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

    level.pawnCount = factionCount + factionCount * config.shipCount; // Bases + ships
    level.pawns = MemPtrNew(sizeof(Pawn) * level.pawnCount);
    MemSet(level.pawns, sizeof(Pawn) * level.pawnCount, 0);

    MemSet(level.scores, sizeof(Score) * GAMEMECHANICS_MAXPLAYERCOUNT, 0);

    switch (config.placementStrategy) {
        case PLAYERPLACEMENTSTRATEGY_CORNERS:
        level_applyPlacementCorners(&level, config);
        break;
    }

    return level;
}

LEVEL_SECTION
void level_destroy(Level *level) {
    if (level->pawns != NULL) {
        level->pawnCount = 0;
        MemPtrFree(level->pawns);
    }
    if (level->gridTexts != NULL) {
        level->gridTextCount = 0;
        MemPtrFree(level->gridTexts);
    }
    if (level->actionTiles != NULL) {
        level->actionTileCount = 0;
        MemPtrFree(level->actionTiles);
    }
}