#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>

#include "bottomMenu.h"
#include "colors.h"
#include "cpuLogic.h"
#include "inputPen.h"
#include "level.h"
#include "models.h"
#include "spriteLibrary.h"

typedef enum TargetSelectionType {
    TARGETSELECTIONTYPE_MOVE,
    TARGETSELECTIONTYPE_PHASER,
    TARGETSELECTIONTYPE_TORPEDO
} TargetSelectionType;

typedef enum MenuScreenType {
    MENUSCREEN_START,
    MENUSCREEN_PLAYERCONFIG,
    MENUSCREEN_GAME,
    MENUSCREEN_SCORE,
    MENUSCREEN_RANK,
    MENUSCREEN_RANK_AFTERGAME
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
    Boolean shouldDrawButtons;
    Boolean requiresPauseAfterLayout;

    Coordinate miniMapDrawPosition;
    Coordinate miniMapSize;
    Boolean awaitingEndMiniMapScrolling;

    Coordinate barButtonPositions[2];
    int barButtonHeight;
} DrawingState;

typedef struct GameSession {
    GameState state;
    MenuScreenType menuScreenType;
    InputPen lastPenInput;

    Level level;
    UInt8 currentTurn;
    Pawn *activePawn;
    Boolean disableAutoMoveToNextPawn;
    Pawn cameraPawn;

    Char cpuActionText[15];

    Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT];
    int factionCount;
    int factionTurn;
    CPUStrategyResult *cpuStrategyResult;

    HighlightTile *highlightTiles;
    int highlightTileCount;
    DrawingState drawingState;

    TargetSelectionType targetSelectionType;

    Coordinate viewportOffset;

    Boolean diaSupport;    // Support for large screens that can hide the input area like the T3/T5 ...
    Boolean colorSupport;  // Support for color screens

    Boolean continueCPUPlay;
    Boolean paused;

    Button *displayButtons;
    UInt8 displayButtonCount;

    Movement *movement;
    AttackAnimation *attackAnimation;
    WarpAnimation warpAnimation;
    ShockWaveAnimation *shockWaveAnimation;
    SceneAnimation *sceneAnimation;

    Int32 nextSceneAnimationLaunchTimestamp;
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
DmResID gameSession_menuTopTitleResource();
Boolean gameSession_useValueForBottomTitle();
int gameSession_valueForBottomTitle();
DmResID gameSession_menuBottomTitleResource();
Pawn *gameSession_pawnAtTile(Coordinate tile);

#endif
