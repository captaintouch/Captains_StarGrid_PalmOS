#include "hexgrid.h"
#include "../constants.h"
#include "colors.h"
#include "drawhelper.h"
#include "mathIsFun.h"
#include "models.h"
#include "../constants.h"

#define HEXGRID_ROWS 15
#define HEXGRID_COLS 15
#define HEXTILE_POINTS 6

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

static void hexgrid_drawPixel4Bit(UInt8 *framebuffer, UInt16 screenWidth, int x, int y, UInt8 colorIndex) {
    UInt16 rowBytes = (screenWidth + 1) / 2;
    UInt32 offset = y * rowBytes + (x / 2);
    UInt8 currentByte = framebuffer[offset];
    if (x % 2 == 0) {
        currentByte = (currentByte & 0x0F) | (colorIndex << 4);
    } else {
        currentByte = (currentByte & 0xF0) | (colorIndex & 0x0F);
    }
    framebuffer[offset] = currentByte;
}

static void hexgrid_fillTile(int startX, int startY, AppColor color, WinHandle buffer) {
    UInt8 *framebuffer;
    int x, y;
    
    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(startX, startY, coordinates);

    framebuffer = (void *)BmpGetBits(WinGetBitmap(buffer));

    for (x = 0; x < HEXTILE_SIZE; x++) {
        int actualX = x + startX;
        for (y = 0; y < HEXTILE_SIZE; y++) {
            int actualY = y + startY;
            if (hexgrid_isInsideTile(coordinates, (Coordinate){actualX, actualY})) {
                if (false) {
                    hexgrid_drawPixel4Bit(framebuffer, HEXTILE_SIZE, actualX, actualY, colors_reference[color]);
                } else {
                    framebuffer[actualY * GAMEWINDOW_WIDTH + actualX] = colors_reference[color];
                }
            }
        }
    }
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

void hexgrid_fillTileAtPosition(Coordinate hexPosition, AppColor color, WinHandle buffer) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    hexgrid_fillTile(startPosition.x, startPosition.y, color, buffer);
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

void hexgrid_drawSpriteAtTile(ImageSprite *imageSprite, Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate centerPosition = (Coordinate){startPosition.x + HEXTILE_SIZE / 2, startPosition.y + HEXTILE_SIZE / 2};
    drawhelper_drawSprite(imageSprite, centerPosition);
}