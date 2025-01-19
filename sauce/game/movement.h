#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, UInt8 *orientation);
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate);
void movement_updateValidPawnPositionsForMovement(Coordinate currentPosition, int maxTileRange, Coordinate **results, int *numberOfPositions);
#endif
