#ifndef LEVEL_H_
#define LEVEL_H_
#define LEVEL_SECTION  __attribute__ ((section ("level")))
#include "models.h"

#define MAXPLAYERCOUNT 4

typedef struct PlayerConfig {
    Boolean active;
    Boolean isHuman;
} PlayerConfig;

typedef enum PlayerPlacementStrategy {
    PLAYERPLACEMENTSTRATEGY_CORNERS
} PlayerPlacementStrategy;

typedef struct NewGameConfig {
    PlayerConfig playerConfig[MAXPLAYERCOUNT];
    PlayerPlacementStrategy placementStrategy;
    int shipCount;
} NewGameConfig;

typedef struct GridText {
    Coordinate position;
    int textResource;
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
    ACTIONTILEIDENTIFIER_FOURPLAYERS
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
} Level;

Level level_startLevel() LEVEL_SECTION;
void level_addPlayerConfigPawns(Level *level, NewGameConfig newGameConfig) LEVEL_SECTION;
void level_applyNewGameConfig(NewGameConfig config, Level *level) LEVEL_SECTION;
NewGameConfig level_getNewGameConfig(Level *level, NewGameConfig oldConfig) LEVEL_SECTION;
UInt8 level_factionCount(NewGameConfig config) LEVEL_SECTION;
Level level_create(NewGameConfig config) LEVEL_SECTION;
void level_destroy(Level *level) LEVEL_SECTION;
void level_addPawn(Pawn pawn, Level *level) LEVEL_SECTION;
#endif