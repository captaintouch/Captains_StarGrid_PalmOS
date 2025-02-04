#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

int movement_distance(Coordinate axialA, Coordinate axialB);
Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage);
Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, int *orientation);
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate);
void movement_findTilesInRange(Coordinate currentPosition, int maxTileRange, Coordinate *invalidCoordinates, int invalidCoordinatesCount, Coordinate **results, int *numberOfPositions);
int movement_orientationBetween(Coordinate coordA, Coordinate coordB);
#endif
