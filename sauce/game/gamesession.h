#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"
#include "colors.h"

typedef struct Pawn {
    Coordinate position;
} Pawn;

typedef enum TargetSelectionType {
    TARGETSELECTIONTYPE_MOVE,
    TARGETSELECTIONTYPE_PHASER,
    TARGETSELECTIONTYPE_TORPEDO
} TargetSelectionType;

typedef enum GameState {
    GAMESTATE_SELECTTARGET,
    GAMESTATE_DEFAULT
} GameState;

typedef struct GameSession {
    GameState state;
    InputPen lastPenInput;

    Pawn *pawns;
    int pawnCount;
    Pawn *activePawn;

    Coordinate *specialTiles; //Contains the tiles that should be colored to indicate where movement is possible
    int specialTileCount;
    Boolean shouldRedrawOverlay;

    TargetSelectionType targetSelectionType;

    Coordinate viewportOffset; 
} GameSession;

GameSession gameSession;

void gameSession_initialize();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();
AppColor gameSession_specialTilesColor();

#endif