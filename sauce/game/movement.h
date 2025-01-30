#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, UInt8 *orientation);
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate);
void movement_findTilesInRange(Coordinate currentPosition, int maxTileRange, Coordinate *invalidCoordinates, int invalidCoordinatesCount, Coordinate **results, int *numberOfPositions);
#endif
