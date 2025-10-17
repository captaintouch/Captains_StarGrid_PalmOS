#include "gamesession.h"
#include "spriteLibrary.h"
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include "../constants.h"
#include "../deviceinfo.h"
#include "colors.h"
#include "drawhelper.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "models.h"
#include "viewport.h"

#define HEXTILE_POINTS 6
int hexgrid_tilePattern[HEXTILE_SIZE];

HEXGRID_SECTION
void hexgrid_cleanup() {
}

HEXGRID_SECTION
static void hexgrid_tileCoords(int startX, int startY, Coordinate coordinates[], Boolean doubleSize) {
    int hexTileSize = doubleSize ? HEXTILE_SIZE * 2 : HEXTILE_SIZE;
    int hexTileSegmentSize = doubleSize ? HEXTILE_SEGMENT_SIZE * 2 : HEXTILE_SEGMENT_SIZE;
    coordinates[0] = (Coordinate){startX + hexTileSize / 2, startY};
    coordinates[1] = (Coordinate){startX, startY + hexTileSegmentSize};
    coordinates[2] = (Coordinate){startX, startY + hexTileSize - hexTileSegmentSize};
    coordinates[3] = (Coordinate){coordinates[0].x, startY + hexTileSize};
    coordinates[4] = (Coordinate){startX + hexTileSize, coordinates[2].y};
    coordinates[5] = (Coordinate){startX + hexTileSize, coordinates[1].y};
}

HEXGRID_SECTION
static void hexgrid_drawTile(int startX, int startY) {
    int i;
    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(startX, startY, coordinates, false);

    for (i = 0; i < HEXTILE_POINTS; i++) {
        int otherIndex = i == 0 ? HEXTILE_POINTS - 1 : i - 1;
        drawhelper_drawLineBetweenCoordinates(coordinates[i], coordinates[otherIndex]);
    }
}

HEXGRID_SECTION
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

HEXGRID_SECTION
void hexgrid_initialize() {
    int y;
    Coordinate coordinates[HEXTILE_POINTS];

    hexgrid_tileCoords(0, 0, coordinates, false);
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

HEXGRID_SECTION
static Coordinate hexgrid_tileStartPosition(int column, int row) {
    if (row % 2 != 0) {
        return (Coordinate){column * HEXTILE_SIZE, row * HEXTILE_SIZE - (row * HEXTILE_SEGMENT_SIZE)};
    } else {
        return (Coordinate){column * HEXTILE_SIZE + HEXTILE_SIZE / 2, row * HEXTILE_SIZE - HEXTILE_SEGMENT_SIZE - ((row - 1) * HEXTILE_SEGMENT_SIZE)};
    }
}

HEXGRID_SECTION
Coordinate hexgrid_tileCenterPosition(Coordinate tilePosition) {
    Coordinate position = hexgrid_tileStartPosition(tilePosition.x, tilePosition.y);
    return (Coordinate){position.x + HEXTILE_SIZE / 2, position.y + HEXTILE_SIZE / 2};
}

HEXGRID_SECTION
void hexgrid_drawTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }
    hexgrid_drawTile(startPosition.x, startPosition.y);
}

HEXGRID_SECTION
void hexgrid_fillTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport, FilledTileType tileType) {
    Coordinate startPosition = hexgrid_tileCenterPosition(hexPosition);
    ImageSprite* sprite;
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }
    switch (tileType) {
        case FILLEDTILETYPE_FEATURED:
            sprite = &spriteLibrary.tileFeaturedSprite;
            break;
        case FILLEDTILETYPE_MOVE:
            sprite = &spriteLibrary.tileMoveSprite;
            break;
        case FILLEDTILETYPE_WARN:
        case FILLEDTILETYPE_WARNEXCLAMATION:
            sprite = &spriteLibrary.tileWarnSprite;
            break;
        case FILLEDTILETYPE_ATTACK:
            sprite = &spriteLibrary.tileAttackSprite;
            break;
    }
    drawhelper_drawSprite(sprite, startPosition);
}

HEXGRID_SECTION
void hexgrid_drawEntireGrid(Boolean adjustForViewport) {
    int i, j;
    Coordinate screenSize = deviceinfo_screenSize();
    drawhelper_applyForeColor(ASBESTOS);
    for (i = 0; i < HEXGRID_COLS; i++) {
        for (j = 0; j < HEXGRID_ROWS; j++) {
            Coordinate targetPosition = hexgrid_tileStartPosition(i, j);
            if (adjustForViewport) {
                targetPosition = viewport_convertedCoordinate(targetPosition);
                if (targetPosition.x + HEXTILE_SIZE < 0 || targetPosition.y + HEXTILE_SIZE < 0 || targetPosition.x - HEXTILE_SIZE > screenSize.x || targetPosition.y - HEXTILE_SIZE > screenSize.y - BOTTOMMENU_HEIGHT) {
                    continue;
                }
            }
            hexgrid_drawTile(targetPosition.x, targetPosition.y);
        }
    }
}

HEXGRID_SECTION
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

HEXGRID_SECTION
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
            if (x >= tileCoordinate.x + hexgrid_tilePattern[yIndex] && x <= tileCoordinate.x + HEXTILE_SIZE - hexgrid_tilePattern[yIndex]) {
                return (Coordinate){c, r};
            }
        }
    }
    return (Coordinate){-1, -1};
}

HEXGRID_SECTION
Coordinate hexgrid_size() {
    Coordinate lastPosition = hexgrid_tileCenterPosition((Coordinate){HEXGRID_COLS - 1, HEXGRID_ROWS - 1});
    return (Coordinate){lastPosition.x + HEXTILE_SIZE + 5, lastPosition.y + HEXTILE_SIZE + 5};
}

HEXGRID_SECTION
void hexgrid_drawSpriteAtTile(ImageSprite* imageSprite, Coordinate hexPosition, Boolean adjustForViewport) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate centerPosition = (Coordinate){startPosition.x + HEXTILE_SIZE / 2, startPosition.y + HEXTILE_SIZE / 2};
    drawhelper_drawSprite(imageSprite, adjustForViewport ? viewport_convertedCoordinate(centerPosition) : centerPosition);
}
