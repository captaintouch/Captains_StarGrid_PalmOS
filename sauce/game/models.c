#include "models.h"
#include "../constants.h"

static Pawn nullPawn = {PAWNTYPE_SHIP, {-1, -1}, {-1, 0, 0, 0, BASEACTION_NONE, false}, 0, 0, false, false};

Pawn *validPawn(Pawn *pawn) {
    return pawn != NULL ? pawn : &nullPawn;
}

Boolean isPawnSet(Pawn *pawn) {
    return pawn != NULL && pawn != &nullPawn;
}

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB) {
    return coordA.x == coordB.x && coordA.y == coordB.y;
}

Boolean isInvalidCoordinate(Coordinate coord) {
    return coord.x < 0 || coord.y < 0;
}

Boolean isPositionInBounds(Coordinate coord) {
    return !(coord.x < 0 || coord.y < 0 || coord.x >= HEXGRID_COLS || coord.y >= HEXGRID_ROWS);
}