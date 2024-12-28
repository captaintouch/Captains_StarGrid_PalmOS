#include "hexgrid.h"

#include "../constants.h"
#include "colors.h"
#include "drawhelper.h"
#include "mathIsFun.h"
#include "models.h"

#define HEXGRID_ROWS 15
#define HEXGRID_COLS 15

static void hexgrid_drawTile(int startX, int startY) {
    Coordinate coordA = {startX + HEXTILE_SIZE / 2, startY};
    Coordinate coordB = {startX, startY + HEXTILE_SEGMENT_SIZE};
    Coordinate coordC = {startX, startY + HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE};
    Coordinate coordD = {coordA.x, startY + HEXTILE_SIZE};
    Coordinate coordE = {startX + HEXTILE_SIZE, coordC.y};
    Coordinate coordF = {startX + HEXTILE_SIZE, coordB.y};

    drawhelper_drawLineBetweenCoordinates(coordA, coordB);
    drawhelper_drawLineBetweenCoordinates(coordB, coordC);
    drawhelper_drawLineBetweenCoordinates(coordC, coordD);
    drawhelper_drawLineBetweenCoordinates(coordD, coordE);
    drawhelper_drawLineBetweenCoordinates(coordE, coordF);
    drawhelper_drawLineBetweenCoordinates(coordF, coordA);
}

static Coordinate hexgrid_tileStartPosition(int column, int row) {
    if (row % 2 == 0) {
        return (Coordinate){column * HEXTILE_SIZE, row * HEXTILE_SIZE - (row * HEXTILE_SEGMENT_SIZE)};
    } else {
        return (Coordinate){column * HEXTILE_SIZE + HEXTILE_SIZE / 2, row * HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE - ((row - 1) * HEXTILE_SEGMENT_SIZE)};
    }
}

void hexgrid_drawTileAtPosition(Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    hexgrid_drawTile(startPosition.x, startPosition.y);
}

void hexgrid_drawEntireGrid() {
    int i, j;
    for (i = 0; i < HEXGRID_COLS; i++) {
        for (j = 0; j < HEXGRID_ROWS; j++) {
            hexgrid_drawTileAtPosition((Coordinate){i, j});
        }
    }
}

static Coordinate hexgrid_centerCoordinateForTile(int column, int row) {
    Coordinate startPosition = hexgrid_tileStartPosition(column, row);
    return (Coordinate){startPosition.x + HEXTILE_SIZE / 2, startPosition.y + HEXTILE_SIZE / 2};
}

static int hexgrid_estimatedRow(float y) {
    int row = 0;
    double offset = 0;

    while (y > offset) {
        offset += HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE;
        row++;
    }
    row--;

    return row;
}

Coordinate hexgrid_tileAtPixel(float x, float y) {
    int r, c;
    Coordinate closestCell;
    float distance = 9999.0;
    int estimatedRow = hexgrid_estimatedRow(y);
    for (r = fmax(0, estimatedRow); r < fmin(HEXGRID_ROWS, estimatedRow + 2); r++) {
        for (c = 0; c < HEXGRID_COLS; c++) {
            float dx, dy, newdistance;
            Coordinate tileCoordinate = hexgrid_centerCoordinateForTile(c, r);
            dx = x - (float)tileCoordinate.x;
            dy = y - (float)tileCoordinate.y;
            newdistance = (float)sqrt(dx * dx + dy * dy);

            if (newdistance < distance) {
                distance = newdistance;
                closestCell = (Coordinate){c, r};
            }
        }
    }
    return closestCell;
}
