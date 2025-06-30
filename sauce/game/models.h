#ifndef MODELS_H_
#define MODELS_H_
#include <PalmOS.h>
#include "colors.h"

#define WARPCIRCLECOUNT 3

typedef struct Coordinate {
    int x;
    int y;
} Coordinate;

typedef struct Line {
    Coordinate startpoint;
    Coordinate endpoint;
} Line;

typedef struct Trajectory {
    Coordinate* tileCoordinates;
    UInt8 tileCount;
} Trajectory;

typedef enum {
    PAWNTYPE_SHIP,
    PAWNTYPE_BASE
} PawnType;

typedef enum {
    BASEACTION_NONE,
    BASEACTION_SHOCKWAVE,
    BASEACTION_BUILD_SHIP,
} BaseAction;

typedef struct Inventory {
    int health;
    UInt8 flagOfFaction;
    UInt8 torpedoCount;
    UInt8 baseActionLastActionTurn; // last round when shockwave or build ship was used
    BaseAction lastBaseAction;
    Boolean carryingFlag;
} Inventory;

typedef struct Pawn {
    PawnType type;
    Coordinate position;
    Inventory inventory;
    UInt8 orientation;
    UInt8 faction;
    Boolean warped;
    Boolean turnComplete;
} Pawn;

typedef struct AttackAnimation {
    Coordinate target;
    Coordinate torpedoPosition;
    Coordinate explosionPosition;
    Int32 launchTimestamp;
    Int32 explosionTimestamp;
    float explosionDurationSeconds;
    float durationSeconds;
    Line *lines;
    Pawn *targetPawn;
    UInt8 lineCount;
    UInt8 healthImpact;
} AttackAnimation;

typedef struct WarpAnimation {
    Pawn *pawn;
    Coordinate currentPosition;
    Coordinate endPosition;
    int circleDiameter[WARPCIRCLECOUNT];
    Int32 launchTimestamp;
    Boolean isWarping;
    Boolean shipVisible;
} WarpAnimation;

typedef struct ShockWaveAnimation {
    int *affectedPawnIndices;
    int affectedPawnCount;
    Coordinate *pawnOriginalPositions;
    Coordinate *pawnIntermediatePositions;
    Pawn *basePawn;
    int circleDiameter[WARPCIRCLECOUNT];
    int maskCircleDiameter;
    Int32 launchTimestamp;
} ShockWaveAnimation;

typedef struct Movement {
    Pawn *pawn;
    Pawn *targetPawn;
    Trajectory trajectory;
    Int32 launchTimestamp;
    Coordinate pawnPosition;
} Movement;

typedef struct HighlightTile {
    Coordinate position;
    AppColor color;
    Boolean filled;
} HighlightTile;

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB);
Boolean isInvalidCoordinate(Coordinate coord);
Boolean isPositionInBounds(Coordinate coord);

#endif