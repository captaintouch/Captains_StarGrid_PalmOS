#include "level.h"

#include <PalmOS.h>

#include "../constants.h"

Level level_startLevel() {
    // Start screen options:
    // - New
    // - Rank
    // - About

    Level level;
    level.pawns = MemPtrNew(sizeof(Pawn) * 4);
    level.actionTileCount = 0;
    level.actionTiles = NULL;
    MemSet(level.pawns, sizeof(Pawn) * 4, 0);
    level.pawnCount = 4;
    level.pawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 3, false, false};
    level.pawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 0, false, false};
    level.pawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 1, false, false};
    level.pawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 2, false, false};

    level.gridTexts = MemPtrNew(sizeof(GridText) * 3);
    MemSet(level.gridTexts, sizeof(GridText) * 3, 0);
    level.gridTextCount = 3;
    level.gridTexts[0] = (GridText){STRING_NEW, (Coordinate){1, 2}, false};
    level.gridTexts[1] = (GridText){STRING_RANK, (Coordinate){1, 4}, false};
    level.gridTexts[2] = (GridText){STRING_ABOUT, (Coordinate){1, 6}, false};

    return level;
}

void level_applyNewGameConfig(NewGameConfig config, Level *level) {
    int i;
    int activePlayers = 0;
    for (i = 0; i < MAXPLAYERCOUNT; i++) {
        if (config.playerConfig[i].active) {
            activePlayers++;
        }
    }

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

NewGameConfig level_getNewGameConfig(Level *level) {
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

    for (i = 0; i < MAXPLAYERCOUNT; i++) {
        config.playerConfig[i].active = i < activePlayers;
    }

    return config;
}

void level_addPlayerConfigPawns(Level *level, NewGameConfig newGameConfig) {
    int i;
    int index;
    int pawnCount = level->pawnCount;
    int additionalPawnCount = 4;
    Pawn *updatedPawns = MemPtrNew(sizeof(Pawn) * (pawnCount + additionalPawnCount));
    MemSet(updatedPawns, sizeof(Pawn) * (pawnCount + additionalPawnCount), 0);
    MemMove(updatedPawns, level->pawns, sizeof(Pawn) * pawnCount);
    MemPtrFree(level->pawns);
    level->pawns = updatedPawns;

    level->pawns[level->pawnCount] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 0, false, false};
    level->pawns[level->pawnCount + 1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){7, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 1, false, false};
    level->pawns[level->pawnCount + 2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 2, false, false};
    level->pawns[level->pawnCount + 3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){11, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 3, false, false};
    level->pawnCount += additionalPawnCount;

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

    level_applyNewGameConfig(newGameConfig, level);
}

Level level_create() {
    Level level;

    level.pawns = MemPtrNew(sizeof(Pawn) * 9);
    MemSet(level.pawns, sizeof(Pawn) * 9, 0);
    level.pawnCount = 9;
    level.pawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 0, false, false};
    level.pawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 0, false, false};
    level.pawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){9, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 1, true, false};
    level.pawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){10, 7}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 1, false, false};
    level.pawns[4] = (Pawn){PAWNTYPE_SHIP, (Coordinate){1, 9}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 2, false, false};
    level.pawns[5] = (Pawn){PAWNTYPE_SHIP, (Coordinate){2, 10}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 0, 2, false, false};

    level.pawns[6] = (Pawn){PAWNTYPE_BASE, (Coordinate){1, 1}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 0, 0, true}, 0, 0, false, false};
    level.pawns[7] = (Pawn){PAWNTYPE_BASE, (Coordinate){10, 6}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 1, 0, true}, 0, 1, false, false};
    level.pawns[8] = (Pawn){PAWNTYPE_BASE, (Coordinate){1, 10}, (Inventory){GAMEMECHANICS_MAXBASEHEALTH, 2, 0, true}, 0, 2, false, false};

    level.gridTexts = NULL;
    level.gridTextCount = 0;

    level.actionTiles = NULL;
    level.actionTileCount = 0;

    return level;
}

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