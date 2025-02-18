#ifndef MODELS_H_
#define MODELS_H_
#include <PalmOS.h>

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

typedef struct Inventory {
    int health;
    UInt8 flagOfFaction;
    Boolean carryingFlag;
} Inventory;

typedef struct Pawn {
    PawnType type;
    Coordinate position;
    Inventory inventory;
    UInt8 orientation;
    UInt8 faction;
    Boolean cloaked;
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

typedef struct Movement {
    Pawn *pawn;
    Pawn *targetPawn;
    Trajectory trajectory;
    Int32 launchTimestamp;
    Coordinate pawnPosition;
} Movement;

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB);
Boolean isInvalidCoordinate(Coordinate coord);

#endif