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
    int tileCount;
} Trajectory;

typedef enum PawnType {
    PAWNTYPE_SHIP,
    PAWNTYPE_FLAG
} PawnType;

typedef struct Pawn {
    Coordinate position;
    int orientation;
    Boolean cloaked;
    PawnType pawnType;
    int faction;
} Pawn;

typedef struct AttackAnimation {
    Coordinate target;
    Int32 launchTimestamp;
    Line *lines;
    int lineCount;
} AttackAnimation;

typedef struct Movement {
    Pawn *pawn;
    Trajectory trajectory;
    Int32 launchTimestamp;
    Coordinate pawnPosition;
} Movement;

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB);
Boolean isInvalidCoordinate(Coordinate coord);

#endif