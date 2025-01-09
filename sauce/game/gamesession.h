#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"
#include "colors.h"
#include "bottomMenu.h"

typedef struct Pawn {
    Coordinate position;
    Boolean cloaked;
} Pawn;

typedef enum TargetSelectionType {
    TARGETSELECTIONTYPE_MOVE,
    TARGETSELECTIONTYPE_PHASER,
    TARGETSELECTIONTYPE_TORPEDO
} TargetSelectionType;

typedef enum GameState {
    GAMESTATE_SELECTTARGET,
    GAMESTATE_DEFAULT,
    GAMESTATE_CHOOSEPAWNACTION
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

    Boolean diaSupport; // Support for large screens that can hide the input area like the T3/T5 ...

    Button *displayButtons;
    int displayButtonCount;
} GameSession;

GameSession gameSession;

void gameSession_initialize();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();
AppColor gameSession_specialTilesColor();

#endif