#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"

typedef struct Pawn {
    Coordinate position;
} Pawn;

typedef struct GameSession {
    InputPen lastPenInput;

    Pawn *pawns;
    int pawnCount;
    //Pawn *activePawn;

    Coordinate *specialTiles; //Contains the tiles that should be colored to indicate where movement is possible
    int specialTileCount;
} GameSession;

GameSession gameSession;

void gameSession_initialize();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();

#endif