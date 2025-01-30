#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"
#include "colors.h"
#include "bottomMenu.h"

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

typedef struct DrawingState {
    Boolean shouldRedrawBackground;
    Boolean shouldRedrawOverlay;

    Coordinate miniMapDrawPosition;
    Coordinate miniMapSize;
} DrawingState;

typedef struct GameSession {
    GameState state;
    InputPen lastPenInput;

    Pawn *pawns;
    int pawnCount;
    Pawn *activePawn;

    Coordinate *highlightTiles; //Contains the tiles that should be colored to indicate where movement is possible
    int highlightTileCount;
    Coordinate *secondaryHighlightTiles; //Tiles that indicate the range but can't be selected
    int secondaryHighlightTileCount;
    DrawingState drawingState;

    TargetSelectionType targetSelectionType;

    Coordinate viewportOffset; 

    Boolean diaSupport; // Support for large screens that can hide the input area like the T3/T5 ...
    Boolean colorSupport; // Support for color screens

    Button *displayButtons;
    UInt8 displayButtonCount;

    Movement *movement;
} GameSession;

GameSession gameSession;

void gameSession_initialize();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();
AppColor gameSession_hightlightTilesColor();
AppColor gameSession_factionColor(UInt8 faction);

#endif