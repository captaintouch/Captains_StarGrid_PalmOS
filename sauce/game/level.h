#ifndef LEVEL_H_
#define LEVEL_H_
#define LEVEL_SECTION  __attribute__ ((section ("level")))
#include "models.h"
#include "scoring.h"
#include "../constants.h"

typedef struct PlayerConfig {
    Boolean active;
    Boolean isHuman;
} PlayerConfig;

typedef enum PlayerPlacementStrategy {
    PLAYERPLACEMENTSTRATEGY_CORNERS,
    PLAYERPLACEMENTSTRATEGY_CENTERDEFENSE
} PlayerPlacementStrategy;

typedef struct NewGameConfig {
    PlayerConfig playerConfig[GAMEMECHANICS_MAXPLAYERCOUNT];
    PlayerPlacementStrategy placementStrategy;
    int shipCount;
} NewGameConfig;

typedef struct GridText {
    Coordinate position;
    Coordinate textOffset;
    int textResource;
    char fixedText[20];
    Boolean alternateColor;
    Boolean simpleText;
    Boolean filler;
} GridText;

typedef enum ActionTileIdentifier {
    ACTIONTILEIDENTIFIER_HUMANPLAYER,
    ACTIONTILEIDENTIFIER_CPUPLAYER,
    ACTIONTILEIDENTIFIER_LAUNCHGAME,
    ACTIONTILEIDENTIFIER_TWOPLAYERS,
    ACTIONTILEIDENTIFIER_THREEPLAYERS,
    ACTIONTILEIDENTIFIER_FOURPLAYERS,
    ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS,
    ACTIONTILEIDENTIFIER_ENDGAME
} ActionTileIdentifier;

typedef struct ActionTile {
    Coordinate position;
    Boolean selected;
    ActionTileIdentifier identifier;
    Boolean hidden;
    int tag;
} ActionTile;

typedef struct Level {
    Pawn *pawns;
    int pawnCount;
    GridText *gridTexts;
    int gridTextCount;
    ActionTile *actionTiles;
    int actionTileCount;
    LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT];
} Level;

Level level_startLevel() LEVEL_SECTION;
void level_addPlayerConfigPawns(Level *level, NewGameConfig newGameConfig) LEVEL_SECTION;
void level_applyNewGameConfig(NewGameConfig config, Level *level) LEVEL_SECTION;
NewGameConfig level_getNewGameConfig(Level *level, NewGameConfig oldConfig) LEVEL_SECTION;
UInt8 level_factionCount(NewGameConfig config) LEVEL_SECTION;
Level level_create(NewGameConfig config) LEVEL_SECTION;
void level_destroy(Level *level) LEVEL_SECTION;
void level_addPawn(Pawn pawn, Level *level) LEVEL_SECTION;
void level_addScorePawns(Level *level, int faction) LEVEL_SECTION;
void level_addRank(Level *level, Score score) LEVEL_SECTION;
void level_removePawn(Pawn *pawn, Level *level) LEVEL_SECTION;
void level_removePawnAtIndex(int index, Level *level) LEVEL_SECTION;
void level_reorderPawnsByDistance(Level *level) LEVEL_SECTION;
Pawn *level_nextPawn(Pawn *currentPawn, Boolean allPawns, int factionTurn, int currentTurn, Level *level) LEVEL_SECTION;
Boolean level_movesLeftForFaction(int faction, int currentTurn, Level *level) LEVEL_SECTION;
NewGameConfig level_defaultNewGameConfig(int rank) LEVEL_SECTION;

Pawn *level_pawnAtTile(Coordinate tileCoordinate, Level *level) LEVEL_SECTION;
#endif