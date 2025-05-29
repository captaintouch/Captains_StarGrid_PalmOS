#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"
#include "colors.h"
#include "bottomMenu.h"
#include "cpuLogic.h"
#include "level.h"

typedef enum TargetSelectionType {
    TARGETSELECTIONTYPE_MOVE,
    TARGETSELECTIONTYPE_PHASER,
    TARGETSELECTIONTYPE_TORPEDO
} TargetSelectionType;

typedef enum MenuScreenType {
    MENUSCREEN_START,
    MENUSCREEN_PLAYERCONFIG,
    MENUSCREEN_GAME
} MenuScreenType;

typedef enum GameState {
    GAMESTATE_SELECTTARGET,
    GAMESTATE_DEFAULT,
    GAMESTATE_CHOOSEPAWNACTION,
} GameState;

typedef struct DrawingState {
    Boolean shouldRedrawBackground;
    Boolean shouldRedrawHeader;
    Boolean shouldResetGameForm;
    Boolean shouldRedrawOverlay;
    Boolean shouldDrawButtons;
    Boolean requiresPauseAfterLayout;

    Coordinate miniMapDrawPosition;
    Coordinate miniMapSize;
    Boolean awaitingEndMiniMapScrolling;

    Coordinate barButtonPositions[2];
    int barButtonHeight;
} DrawingState;

typedef struct Faction {
    CPUFactionProfile profile;
    Boolean human;
} Faction;

typedef struct GameRestorableSessionData {
    int pawnCount;
    Faction factions[MAXPLAYERCOUNT];
    Score scores[MAXPLAYERCOUNT];
    int factionCount;
    int factionTurn;
    int currentTurn;
} GameRestorableSessionData;

typedef struct GameSession {
    GameState state;
    MenuScreenType menuScreenType;
    InputPen lastPenInput;

    Level level;
    UInt8 currentTurn;
    Pawn *activePawn;
    Pawn cameraPawn;

    Char cpuActionText[15];

    Faction factions[4];
    int factionCount;
    int factionTurn;
    CPUStrategyResult *cpuStrategyResult;

    Coordinate *highlightTiles; //Contains the tiles that should be colored to indicate where movement is possible
    int highlightTileCount;
    Coordinate *secondaryHighlightTiles; //Tiles that indicate the range but can't be selected
    int secondaryHighlightTileCount;
    DrawingState drawingState;

    TargetSelectionType targetSelectionType;

    Coordinate viewportOffset; 

    Boolean diaSupport; // Support for large screens that can hide the input area like the T3/T5 ...
    Boolean colorSupport; // Support for color screens

    Boolean continueCPUPlay;
    Boolean paused;

    Button *displayButtons;
    UInt8 displayButtonCount;

    Movement *movement;
    AttackAnimation *attackAnimation;
    WarpAnimation warpAnimation;
    ShockWaveAnimation *shockWaveAnimation;
} GameSession;

GameSession gameSession;

void gameSession_reset(Boolean newGame);
void gameSession_cleanup();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();
Boolean gameSession_handleMenu(UInt16 menuItemID);
Boolean gameSession_handleFormButtonTap(UInt16 buttonID);
Boolean gameSession_shouldShowHealthBar();
Boolean gameSession_animating();
AppColor gameSession_hightlightTilesColor();
DmResID gameSession_menuTopTitleResource();
DmResID gameSession_menuBottomTitleResource();

#endif