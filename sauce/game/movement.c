#include "movement.h"

#include <PalmOS.h>

#include "../constants.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "models.h"

typedef struct Cube {
    int q;
    int r;
    int s;
} Cube;

static Cube movement_cubeFromCoordinates(Coordinate a) {
    int q = a.x - (a.y + (a.y & 1)) / 2;
    int r = a.y;
    int s = -q - r;
    return (Cube){q, r, s};
}

static Cube movement_cubeSubtract(Cube a, Cube b) {
    return (Cube){a.q - b.q, a.r - b.r, a.s - b.s};
}

int movement_distance(Coordinate axialA, Coordinate axialB) {
    Cube a = movement_cubeFromCoordinates(axialA);
    Cube b = movement_cubeFromCoordinates(axialB);
    Cube vec = movement_cubeSubtract(a, b);
    return (abs(vec.q) + abs(vec.r) + abs(vec.s)) / 2;
}

UInt8 movement_orientationBetween(Coordinate coordA, Coordinate coordB) {
    int dx = coordB.x - coordA.x;
    int dy = coordB.y - coordA.y;

    int direction = 0;
    if (dx < 0 && dy == 0) direction = 0;  // WEST
    if (dx == 0 && dy < 0) {
        if (coordA.y % 2 == coordB.y % 2) {
            direction = 2;  // NORTH
        } else {
            direction = (coordA.y % 2 == 0) ? 1 : 3;  // NORTHWEST : NORTHEAST
        }
    }
    if (dx > 0 && dy == 0) direction = 4;  // EAST
    if (dx == 0 && dy > 0) {
        if (coordA.y % 2 == coordB.y % 2) {
            direction = 6;  // SOUTH
        } else {
            direction = (coordA.y % 2 == 0) ? 7 : 5;  // SOUTHWEST : SOUTHEAST
        }
    }
    if (dx < 0 && dy < 0) direction = 1;  // NORTHWEST
    if (dx > 0 && dy < 0) direction = 3;  // NORTHEAST
    if (dx < 0 && dy > 0) direction = 7;  // SOUTHWEST
    if (dx > 0 && dy > 0) direction = 5;  // SOUTHEAST

    return direction;
}

Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage) {
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
    closestDistance = movement_distance(closestCoordinate, endCoordinate);
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            Coordinate updatedCoordinate = (Coordinate){originCoordinate.x + i, originCoordinate.y + j};
            int updatedDistance = movement_distance(updatedCoordinate, endCoordinate);
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
    int i, j;
    int distance = movement_distance(startCoordinate, endCoordinate);

    trajectory.tileCoordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * 40);
    trajectory.tileCoordinates[0] = (Coordinate){startCoordinate.x, startCoordinate.y};
    trajectory.tileCount = 1;

    for (i = 1; i <= distance; i++) {
        float t = (float)i / distance;
        float q = startCoordinate.x + (endCoordinate.x - startCoordinate.x) * t;
        float r = startCoordinate.y + (endCoordinate.y - startCoordinate.y) * t;

        Coordinate interCoordinate = movement_hexRound(q, r);
        if (movement_isInvalid(trajectory.tileCoordinates[trajectory.tileCount - 1], interCoordinate)) {
            trajectory.tileCoordinates[trajectory.tileCount] = movement_nextManualCoordinate(trajectory.tileCoordinates[trajectory.tileCount - 1], interCoordinate, endCoordinate);
            distance = movement_distance(trajectory.tileCoordinates[trajectory.tileCount], endCoordinate);
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
    for (i = 0; i < fmax(0, trajectory.tileCount - 2); i++) {
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

static Boolean movement_positionInCoordinates(Coordinate referenceCoordinate, Coordinate *invalidCoordinates, int invalidCoordinatesCount) {
    int i;
    if (invalidCoordinates == NULL) {
        return false;
    }
    for (i = 0; i < invalidCoordinatesCount; i++) {
        if (isEqualCoordinate(referenceCoordinate, invalidCoordinates[i])) {
            return true;
        }
    }
    return false;
}

void movement_findTilesInRange(Coordinate currentPosition, int maxTileRange, Coordinate *invalidCoordinates, int invalidCoordinatesCount, Coordinate **results, int *numberOfPositions) {
    int i, j;
    int positionCount = 0;
    Coordinate *positions = (Coordinate *)MemPtrNew(sizeof(Coordinate) * maxTileRange * 2 * maxTileRange * 2);
    for (i = -maxTileRange - 2; i < maxTileRange + 2; i++) {
        for (j = -maxTileRange - 2; j < maxTileRange + 2; j++) {
            Coordinate newPosition = (Coordinate){currentPosition.x + i, currentPosition.y + j};
            if (isEqualCoordinate(currentPosition, newPosition)) {
                continue;
            }
            if (movement_distance(currentPosition, newPosition) > maxTileRange) {
                continue;
            }
            if (invalidCoordinates != NULL && movement_positionInCoordinates(newPosition, invalidCoordinates, invalidCoordinatesCount)) {
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

static Boolean movement_shipAtTarget(Coordinate targetCoordinate, Pawn *allPawns, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        if (allPawns[i].type == PAWNTYPE_SHIP && isEqualCoordinate(allPawns[i].position, targetCoordinate)) {
            return true;
        }
    }
    return false;
}

static Boolean movement_shipOrBaseAtTarget(Coordinate targetCoordinate, Pawn *allPawns, int totalPawnCount) {
    int i;
    for (i = 0; i < totalPawnCount; i++) {
        if (isEqualCoordinate(allPawns[i].position, targetCoordinate)) {
            return true;
        }
    }
    return false;
}

Coordinate movement_closestTileToTargetInRange(Pawn *pawn, Coordinate targetPosition, Pawn *allPawns, int totalPawnCount, Boolean allowBase) {
    Coordinate closestTile = pawn->position;
    int minDistance = 9999;
    int maxRange = GAMEMECHANICS_MAXTILEMOVERANGE;
    int dx, dy;

    for (dx = -maxRange; dx <= maxRange; dx++) {
        for (dy = -maxRange; dy <= maxRange; dy++) {
            Coordinate candidateTile = {pawn->position.x + dx, pawn->position.y + dy};
            Boolean targetOccupied = allowBase ? movement_shipAtTarget(candidateTile, allPawns, totalPawnCount) : movement_shipOrBaseAtTarget(candidateTile, allPawns, totalPawnCount);
            if (!isInvalidCoordinate(candidateTile) && !targetOccupied) {
                int distance = movement_distance(candidateTile, targetPosition);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestTile = candidateTile;
                }
            }
        }
    }

    return closestTile;
}