#ifndef VIEWPORT_H_
#define VIEWPORT_H_
#include "../platform/i_types.h"
#include "models.h"

Coordinate viewport_convertedCoordinate(Coordinate coordinate);
Coordinate viewport_convertedCoordinateInverted(Coordinate coordinate);
Line viewport_convertedLine(Line line);
#endif