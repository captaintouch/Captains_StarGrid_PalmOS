#include "models.h"

Boolean isEqualCoordinate(Coordinate coordA, Coordinate coordB) {
    return coordA.x == coordB.x && coordA.y == coordB.y;
}

Boolean isInvalidCoordinate(Coordinate coord) {
    return coord.x < 0 || coord.y < 0;
}