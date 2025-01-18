#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "models.h"
#include <PalmOS.h>

Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage);
Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate);

#endif
