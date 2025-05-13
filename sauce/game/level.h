#ifndef LEVEL_H_
#define LEVEL_H_
#include "models.h"

typedef struct GridText {
    int textResource;
    Coordinate position;
    Boolean alternateColor;
} GridText;

typedef struct ActionTile {
    Coordinate position;
    Boolean selected;
} ActionTile;

typedef struct Level {
    Pawn *pawns;
    int pawnCount;
    GridText *gridTexts;
    int gridTextCount;
    ActionTile *actionTiles;
    int actionTileCount;
} Level;

// START SCREEN SPECIFIC
Level level_startLevel();
void level_addPlayerConfigPawns(Level *level);

Level level_create();

void level_destroy(Level *level);

#endif