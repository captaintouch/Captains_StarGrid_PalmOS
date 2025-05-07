#include <PalmOS.h>
#include "level.h"
#include "../constants.h"

Level level_startLevel() {

    // Start screen options:
    // - New
    // - Rank
    // - About

    Level level;
    level.pawns = MemPtrNew(sizeof(Pawn) * 4);
    MemSet(level.pawns, sizeof(Pawn) * 4, 0);
    level.pawnCount = 4;
    level.pawns[0] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 2}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 0, false, false};
    level.pawns[1] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 4}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 1, false, false};
    level.pawns[2] = (Pawn){PAWNTYPE_SHIP, (Coordinate){0, 6}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 2, false, false};
    level.pawns[3] = (Pawn){PAWNTYPE_SHIP, (Coordinate){HEXGRID_COLS - HEXGRID_COLS / 3, 0}, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, false}, 4, 3, false, false};

    level.gridTexts = MemPtrNew(sizeof(GridText) * 3);
    MemSet(level.gridTexts, sizeof(GridText) * 3, 0);
    level.gridTextCount = 3;
    level.gridTexts[0] = (GridText){STRING_NEW, (Coordinate){1, 2}};
    level.gridTexts[1] = (GridText){STRING_RANK, (Coordinate){1, 4}};
    level.gridTexts[2] = (GridText){STRING_ABOUT, (Coordinate){1, 6}};

    return level;
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
}