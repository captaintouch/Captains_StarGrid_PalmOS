#include "hexgrid.h"

#include "../constants.h"
#include "colors.h"
#include "drawhelper.h"
#include "mathIsFun.h"
#include "models.h"
#define HEXTILE_POINTS 6

int hexgrid_tilePattern[HEXTILE_SIZE];

static void hexgrid_tileCoords(int startX, int startY, Coordinate coordinates[]) {
    coordinates[0] = (Coordinate){startX + HEXTILE_SIZE / 2, startY};
    coordinates[1] = (Coordinate){startX, startY + HEXTILE_SEGMENT_SIZE};
    coordinates[2] = (Coordinate){startX, startY + HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE};
    coordinates[3] = (Coordinate){coordinates[0].x, startY + HEXTILE_SIZE};
    coordinates[4] = (Coordinate){startX + HEXTILE_SIZE, coordinates[2].y};
    coordinates[5] = (Coordinate){startX + HEXTILE_SIZE, coordinates[1].y};
}

static void hexgrid_drawTile(int startX, int startY) {
    int i;
    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(startX, startY, coordinates);

    for (i = 0; i < HEXTILE_POINTS; i++) {
        int otherIndex = i == 0 ? HEXTILE_POINTS - 1 : i - 1;
        drawhelper_drawLineBetweenCoordinates(coordinates[i], coordinates[otherIndex]);
    }
}

static Boolean hexgrid_isInsideTile(Coordinate coordinates[], Coordinate p) {
    int i, j;
    int isInside = 0;

    for (i = 0, j = HEXTILE_POINTS - 1; i < HEXTILE_POINTS; j = i++) {
        if (((coordinates[i].y > p.y) != (coordinates[j].y > p.y)) &&
            (p.x < (coordinates[j].x - coordinates[i].x) * (p.y - coordinates[i].y) / (coordinates[j].y - coordinates[i].y) + coordinates[i].x)) {
            isInside = !isInside;
        }
    }

    return isInside;
}

void hexgrid_initialize() {
    int y;
    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(0, 0, coordinates);
    for (y = 0; y < HEXTILE_SIZE; y++) {
        int x;
        hexgrid_tilePattern[y] = HEXTILE_SIZE / 2;
        for (x = 0; x < HEXTILE_SIZE; x++) {
            if (hexgrid_isInsideTile(coordinates, (Coordinate){x, y})) {
                hexgrid_tilePattern[y] = x;
                break;
            }
        }
    }
}

static void hexgrid_fillTile(int startX, int startY) {
    int y;

    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(startX, startY, coordinates);

    for (y = 0; y < HEXTILE_SIZE; y++) {
        int actualY = y + startY;
        int xOffset = hexgrid_tilePattern[y];
        if (xOffset < 0) {
            continue;
        }
        drawhelper_drawLineBetweenCoordinates((Coordinate){startX + xOffset, actualY}, (Coordinate){startX + HEXTILE_SIZE - xOffset, actualY});
    }
}

static Coordinate hexgrid_tileStartPosition(int column, int row) {
    if (row % 2 != 0) {
        return (Coordinate){column * HEXTILE_SIZE, row * HEXTILE_SIZE - (row * HEXTILE_SEGMENT_SIZE)};
    } else {
        return (Coordinate){column * HEXTILE_SIZE + HEXTILE_SIZE / 2, row * HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE - ((row - 1) * HEXTILE_SEGMENT_SIZE)};
    }
}

Coordinate hexgrid_tileCenterPosition(Coordinate tilePosition) {
    Coordinate position = hexgrid_tileStartPosition(tilePosition.x, tilePosition.y);
    return (Coordinate){position.x + HEXTILE_SIZE / 2, position.y + HEXTILE_SIZE / 2};
}

void hexgrid_drawTileAtPosition(Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    hexgrid_drawTile(startPosition.x, startPosition.y);
}

void hexgrid_fillTileAtPosition(Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    hexgrid_fillTile(startPosition.x, startPosition.y);
}

void hexgrid_drawEntireGrid() {
    int i, j;
    drawhelper_applyForeColor(ASBESTOS);
    for (i = 0; i < HEXGRID_COLS; i++) {
        for (j = 0; j < HEXGRID_ROWS; j++) {
            hexgrid_drawTileAtPosition((Coordinate){i, j});
        }
    }
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

Coordinate hexgrid_tileAtPixel(int x, int y) {
    int r, c;
    int estimatedRow = hexgrid_estimatedRow(y);
    for (r = fmax(0, estimatedRow - 2); r < fmin(HEXGRID_ROWS, estimatedRow + 2); r++) {
        for (c = 0; c < HEXGRID_COLS; c++) {
            Coordinate tileCoordinate = hexgrid_tileStartPosition(c, r);
            int yIndex = y - tileCoordinate.y;
            if (yIndex < 0 || yIndex >= HEXTILE_SIZE) {
                continue;
            }
            if (x>= tileCoordinate.x + hexgrid_tilePattern[yIndex] && x <= tileCoordinate.x + HEXTILE_SIZE - hexgrid_tilePattern[yIndex]) {
                return (Coordinate){c, r};
            }
        }
    }
    return (Coordinate){-1, -1};
}

Coordinate hexgrid_size() {
    // TODO: calculate exact size
    return (Coordinate) {HEXTILE_SIZE * HEXGRID_COLS, HEXTILE_SIZE * HEXGRID_ROWS};
}

void hexgrid_drawSpriteAtTile(ImageSprite *imageSprite, Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate centerPosition = (Coordinate){startPosition.x + HEXTILE_SIZE / 2, startPosition.y + HEXTILE_SIZE / 2};
    drawhelper_drawSprite(imageSprite, centerPosition);
}