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

typedef struct Pawn {
    Coordinate position;
    UInt8 orientation;
    Boolean cloaked;
    UInt8 faction;
} Pawn;

typedef struct AttackAnimation {
    Coordinate target;
    Int32 launchTimestamp;
    Line *lines;
    UInt8 lineCount;
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