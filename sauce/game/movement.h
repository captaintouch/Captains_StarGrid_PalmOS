#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

int movement_distance(Coordinate axialA, Coordinate axialB);
Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage);
Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, UInt8 *orientation);
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate);
void movement_findTilesInRange(Coordinate currentPosition, int maxTileRange, Coordinate *invalidCoordinates, int invalidCoordinatesCount, Coordinate **results, int *numberOfPositions);
UInt8 movement_orientationBetween(Coordinate coordA, Coordinate coordB);
Coordinate movement_closestTileToTargetInRange(Pawn *pawn, Coordinate targetPosition, Pawn *allPawns, int totalPawnCount, Boolean allowBase);
Boolean movement_shipAtTarget(Coordinate targetCoordinate, Pawn *allPawns, int totalPawnCount);
Pawn *movement_homeBase(Pawn *pawn, Pawn *allPawns, int totalPawnCount);
#endif
