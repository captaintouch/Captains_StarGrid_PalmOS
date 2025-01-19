#include "movement.h"
#include <PalmOS.h>
#include "hexgrid.h"
#include "mathIsFun.h"
#include "models.h"
#include "../constants.h"

static UInt8 movement_orientationBetween(Coordinate coordA, Coordinate coordB) {
    int dx = coordB.x - coordA.x;
    int dy = coordB.y - coordA.y;

    int direction = 0;
    if (dx < 0 && dy == 0) direction = 0; //WEST
    if (dx == 0 && dy < 0) direction = 2; //NORTH
    if (dx > 0 && dy == 0) direction = 4; //EAST
    if (dx == 0 && dy > 0) direction = 6; //SOUTH
    if (dx < 0 && dy < 0) direction = 1; //NORTHWEST
    if (dx > 0 && dy < 0) direction = 3; //NORTHEAST
    if (dx < 0 && dy > 0) direction = 7; //SOUTHWEST
    if (dx > 0 && dy > 0) direction = 5; //SOUTHEAST

    // TODO: offset direction to take into account even/uneven rows which are not aligned
    return direction;
}

static Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage) {
    Coordinate coordinate;
    coordinate.x = line.startpoint.x + percentage * (line.endpoint.x - line.startpoint.x);
    coordinate.y = line.startpoint.y + percentage * (line.endpoint.y - line.startpoint.y);
    return coordinate;
}

Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage, UInt8 *orientation) {
    float easingPercentage = 1.0 - (1.0 - percentage) * (1.0 - percentage);  // Use easing to make progress faster at the start and slower at the end
    float totalDistance = trajectory.tileCount - 1;
    float targetDistance = totalDistance * easingPercentage;
    int index = floor(targetDistance);  // point is between tile[index] and tile[index + 1]
    float diffDistance = targetDistance - index;
    *orientation = movement_orientationBetween(trajectory.tileCoordinates[index], trajectory.tileCoordinates[index + 1]);
    return movement_coordinateAtPercentageOfLine(
        (Line){
            hexgrid_tileCenterPosition(trajectory.tileCoordinates[index]),
            hexgrid_tileCenterPosition(trajectory.tileCoordinates[index + 1])},
        diffDistance);
}

static Coordinate movement_hexRound(float q, float r) {
    float z = -q - r;
    int rq = round(q);
    int rr = round(r);
    int rz = round(z);

    float q_diff = fabs(rq - q);
    float r_diff = fabs(rr - r);
    float z_diff = fabs(rz - z);

    if (q_diff > r_diff && q_diff > z_diff) {
        rq = -rr - rz;
    } else if (r_diff > z_diff) {
        rr = -rq - rz;
    }

    return (Coordinate){rq, rr};
}

static int movement_hexDistance(Coordinate a, Coordinate b) {
    return (abs(a.x - b.y) + abs(a.y - b.y) + abs(-a.x - a.y + b.x + b.y)) / 2;
}

static Boolean movement_isInvalid(Coordinate originCoordinate, Coordinate interCoordinate) {
    int deltaX = interCoordinate.x - originCoordinate.x;
    int deltaY = interCoordinate.y - originCoordinate.y;
    if (abs(deltaX) > 1 || abs(deltaY) > 1) {
        return true;
    }
    if (isEqualCoordinate(originCoordinate, interCoordinate)) {
        return true;
    }
    if (originCoordinate.y % 2 != 0) {
        if ((deltaX == 1 && deltaY == -1) || (deltaX == 1 && deltaY == 1)) {
            return true;
        }
    } else {
        if ((deltaX == -1 && deltaY == -1) || (deltaX == -1 && deltaY == 1)) {
            return true;
        }
    }
    return false;
}

static Boolean movement_isAdjacent(Coordinate coordA, Coordinate coordB) {
    int dx = abs(coordA.x - coordB.x);
    int dy = abs(coordA.y - coordB.y);

    return !movement_isInvalid(coordA, coordB) && (dx <= 1 && dy <= 1);
}

static Coordinate movement_nextManualCoordinate(Coordinate originCoordinate, Coordinate invalidCoordinate, Coordinate endCoordinate) {
    int i, j;
    Coordinate closestCoordinate = originCoordinate;
    int closestDistance;
    if (movement_isAdjacent(originCoordinate, endCoordinate)) {
        return endCoordinate;
    }
    if (originCoordinate.y == endCoordinate.y && originCoordinate.x != endCoordinate.x) {
        int xOffset = (endCoordinate.x > originCoordinate.x) ? 1 : -1;
        return (Coordinate){originCoordinate.x + xOffset, originCoordinate.y};
    }
    if (abs(originCoordinate.y - endCoordinate.y) == 1) {
        int yOffset = (endCoordinate.y > originCoordinate.y) ? 1 : -1;
        return (Coordinate){originCoordinate.x, originCoordinate.y + yOffset};
    }
    closestDistance = movement_hexDistance(closestCoordinate, endCoordinate);
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            Coordinate updatedCoordinate = (Coordinate){originCoordinate.x + i, originCoordinate.y + j};
            int updatedDistance = movement_hexDistance(updatedCoordinate, endCoordinate);
            if (movement_isInvalid(originCoordinate, updatedCoordinate) || isEqualCoordinate(invalidCoordinate, updatedCoordinate) || isEqualCoordinate(invalidCoordinate, originCoordinate)) {
                continue;
            } else if (isEqualCoordinate(endCoordinate, updatedCoordinate)) {
                return updatedCoordinate;
            } else if (updatedDistance <= closestDistance) {
                closestDistance = updatedDistance;
                closestCoordinate = updatedCoordinate;
            }
        }
    }
    return closestCoordinate;
}

Trajectory movement_trajectoryBetween(Coordinate startCoordinate, Coordinate endCoordinate) {
    Trajectory trajectory;
    int i,j;
    int distance = movement_hexDistance(startCoordinate, endCoordinate);

    trajectory.tileCoordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * 40);
    trajectory.tileCoordinates[0] = (Coordinate){startCoordinate.x, startCoordinate.y };
    trajectory.tileCount = 1;

    for (i = 1; i <= distance; i++) {
        float t = (float)i / distance;
        float q = startCoordinate.x + (endCoordinate.x - startCoordinate.x) * t;
        float r = startCoordinate.y + (endCoordinate.y - startCoordinate.y) * t;

        Coordinate interCoordinate = movement_hexRound(q, r);
        if (movement_isInvalid(trajectory.tileCoordinates[trajectory.tileCount - 1], interCoordinate)) {
            trajectory.tileCoordinates[trajectory.tileCount] = movement_nextManualCoordinate(trajectory.tileCoordinates[trajectory.tileCount - 1], interCoordinate, endCoordinate);
            distance = movement_hexDistance(trajectory.tileCoordinates[trajectory.tileCount], endCoordinate);
            startCoordinate = trajectory.tileCoordinates[trajectory.tileCount];
            i = 1;
            trajectory.tileCount++;
            if (movement_isAdjacent(trajectory.tileCoordinates[trajectory.tileCount - 1], endCoordinate)) {
                trajectory.tileCoordinates[trajectory.tileCount] = endCoordinate;
                trajectory.tileCount++;
            }
            break;
        } else {
            trajectory.tileCoordinates[i] = interCoordinate;
            trajectory.tileCount++;
            if (isEqualCoordinate(trajectory.tileCoordinates[trajectory.tileCount - 1], endCoordinate)) {
                break;
            }
            if (movement_isAdjacent(trajectory.tileCoordinates[trajectory.tileCount - 1], endCoordinate)) {
                trajectory.tileCoordinates[trajectory.tileCount] = endCoordinate;
                trajectory.tileCount++;
                break;
            }
        }
    }

    while (!isEqualCoordinate(trajectory.tileCoordinates[trajectory.tileCount - 1], endCoordinate)) {
        trajectory.tileCoordinates[trajectory.tileCount] = movement_nextManualCoordinate(trajectory.tileCoordinates[trajectory.tileCount - 1], (Coordinate){-1, -1}, endCoordinate);
        trajectory.tileCount++;

        if (movement_isAdjacent(trajectory.tileCoordinates[trajectory.tileCount - 1], endCoordinate)) {
            trajectory.tileCoordinates[trajectory.tileCount] = endCoordinate;
            trajectory.tileCount++;
            break;
        }
    }

    // Delete unneccessary tiles
    for (i = 0; i < fmax(0, trajectory.tileCount -2); i++) {
        if (trajectory.tileCount <= 2) {
            return;
        }
        if (!movement_isInvalid(trajectory.tileCoordinates[i], trajectory.tileCoordinates[i + 2])) {
            for (j = i + 1; j < trajectory.tileCount - 1; j++) {
                trajectory.tileCoordinates[j] = trajectory.tileCoordinates[j + 1];
            }
            trajectory.tileCount--;
            i = 0;
        }
    }

    MemPtrResize(trajectory.tileCoordinates, trajectory.tileCount * sizeof(Coordinate));
    return trajectory;
}

void movement_updateValidPawnPositionsForMovement(Coordinate currentPosition, int maxTileRange, Coordinate **results, int *numberOfPositions) {
    int i, j;
    int positionCount = 0;
    Coordinate *positions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * maxTileRange * 2 * maxTileRange * 2);
    for (i = -maxTileRange + 1; i < maxTileRange; i++) {
        for (j = -maxTileRange + 1; j < maxTileRange; j++) {
            Coordinate newPosition = (Coordinate){currentPosition.x + i, currentPosition.y + j};
            if (newPosition.x == currentPosition.x && newPosition.y == currentPosition.y) {
                continue;
            }
            if (newPosition.x >= 0 && newPosition.x < HEXGRID_COLS && newPosition.y >= 0 && newPosition.y < HEXGRID_ROWS) {
                positions[positionCount] = newPosition;
                positionCount++;
            }
        }
    }
    MemPtrResize(positions, positionCount * sizeof(Coordinate));
    *results = positions;
    *numberOfPositions = positionCount;
}
