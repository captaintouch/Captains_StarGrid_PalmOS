#include "viewport.h"
#include <PalmOS.h>
#include "gamesession.h"
#include "../constants.h"

Coordinate viewport_convertedCoordinate(Coordinate coordinate) {
    coordinate.x = -gameSession.viewportOffset.x + coordinate.x;
    coordinate.y = -gameSession.viewportOffset.y + coordinate.y;
    return coordinate;
}

Coordinate viewport_convertedCoordinateInverted(Coordinate coordinate) {
    coordinate.x = gameSession.viewportOffset.x + coordinate.x;
    coordinate.y = gameSession.viewportOffset.y + coordinate.y;
    return coordinate;
}