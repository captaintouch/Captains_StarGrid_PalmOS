#include "models.h"
#include "../constants.h"

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB) {
    return coordA.x == coordB.x && coordA.y == coordB.y;
}

Boolean isInvalidCoordinate(Coordinate coord) {
    return coord.x < 0 || coord.y < 0;
}

Boolean isPositionInBounds(Coordinate coord) {
    return coord.x < 0 || coord.y < 0 || coord.x >= HEXGRID_COLS || coord.y >= HEXGRID_ROWS;
}