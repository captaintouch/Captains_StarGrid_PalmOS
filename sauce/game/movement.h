#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

#define MOVEMENT_SECTION  __attribute__ ((section ("movement")))

int movement_distance(Coordinate axialA, Coordinate axialB) MOVEMENT_SECTION;
Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage) MOVEMENT_SECTION;
Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, UInt8 *orientation) MOVEMENT_SECTION;
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate) MOVEMENT_SECTION;
void movement_findTilesInRange(Coordinate currentPosition, int maxTileRange, Coordinate *invalidCoordinates, int invalidCoordinatesCount, Coordinate **results, int *numberOfPositions) MOVEMENT_SECTION;
UInt8 movement_orientationBetween(Coordinate coordA, Coordinate coordB) MOVEMENT_SECTION;
Coordinate movement_closestTileToTargetInRange(Pawn *pawn, Coordinate targetPosition, Pawn *allPawns, int totalPawnCount, Boolean allowBase) MOVEMENT_SECTION;
Boolean movement_shipAtTarget(Coordinate targetCoordinate, Pawn *allPawns, int totalPawnCount) MOVEMENT_SECTION;
Pawn *movement_homeBase(Pawn *pawn, Pawn *allPawns, int totalPawnCount) MOVEMENT_SECTION;
Coordinate movement_positionAwayFrom(Coordinate sourceCoordinate, Pawn *pawn, Pawn *allPawns, int totalPawnCount, UInt8 distance) MOVEMENT_SECTION;
Coordinate movement_getBoxCoordinate(Coordinate center, float t, int boxSize) MOVEMENT_SECTION;
#endif
