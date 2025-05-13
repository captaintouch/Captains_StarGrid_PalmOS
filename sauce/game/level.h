#ifndef LEVEL_H_
#define LEVEL_H_
#include "models.h"

typedef struct GridText {
    int textResource;
    Coordinate position;
    Boolean alternateColor;
} GridText;

typedef struct Level {
    Pawn *pawns;
    int pawnCount;
    GridText *gridTexts;
    int gridTextCount;
} Level;

Level level_startLevel();
Level level_create();

void level_destroy(Level *level);

#endif