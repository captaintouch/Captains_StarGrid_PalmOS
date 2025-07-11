#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include "hexgrid.h"

#include "../constants.h"
#include "../deviceinfo.h"
#include "colors.h"
#include "drawhelper.h"
#include "mathIsFun.h"
#include "models.h"
#include "viewport.h"
#define HEXTILE_POINTS 6

WinHandle hexgrid_filledTileCacheWindow = NULL;
IndexedColorType hexgrid_filledTileCacheWindowColor = 99;

int hexgrid_tilePattern[HEXTILE_SIZE];
#ifdef HIRESBUILD
int hexgrid_tilePatternDouble[HEXTILE_SIZE * 2];
#endif

HEXGRID_SECTION
void hexgrid_cleanup() {
    if (hexgrid_filledTileCacheWindow != NULL) {
        WinDeleteWindow(hexgrid_filledTileCacheWindow, false);
        hexgrid_filledTileCacheWindow = NULL;
    }
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

#ifndef HIRESBUILD
HEXGRID_SECTION
static Boolean hexgrid_drawTileFromCache(int startX, int startY, WinHandle drawWindow, WinHandle tileWindow, IndexedColorType cacheColor) {
    RectangleType rect;
    if (tileWindow == NULL || tileWindow->drawStateP->foreColor != cacheColor) {
        return false;
    }
    RctSetRectangle(&rect, 0, 0, HEXTILE_SIZE + 1, HEXTILE_SIZE + 1);
    WinCopyRectangle(tileWindow, drawWindow, &rect, startX, startY, winPaint);
    return true;
}

HEXGRID_SECTION
static Boolean hexgrid_createFilledTileCache(WinHandle drawWindow) {
    Err err = errNone;
    if (hexgrid_filledTileCacheWindow != NULL) {
        WinDeleteWindow(hexgrid_filledTileCacheWindow, false);
    }
    hexgrid_filledTileCacheWindow = WinCreateOffscreenWindow(HEXTILE_SIZE + 1, HEXTILE_SIZE + 1, nativeFormat, &err);
    if (err != errNone) {
        hexgrid_filledTileCacheWindow = NULL;
    } else {
        WinSetDrawWindow(hexgrid_filledTileCacheWindow);
        hexgrid_filledTileCacheWindow->bitmapP->flags.hasTransparency = true;
        hexgrid_filledTileCacheWindowColor = hexgrid_filledTileCacheWindow->drawStateP->foreColor;
    }
    return hexgrid_filledTileCacheWindow != NULL;;
}

#endif

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

#ifdef HIRESBUILD
    hexgrid_tileCoords(0, 0, coordinates, true);
    for (y = 0; y < HEXTILE_SIZE * 2; y++) {
        int x;
        hexgrid_tilePatternDouble[y] = HEXTILE_SIZE;
        for (x = 0; x < HEXTILE_SIZE * 2; x++) {
            if (hexgrid_isInsideTile(coordinates, (Coordinate){x, y})) {
                hexgrid_tilePatternDouble[y] = x;
                break;
            }
        }
    }
#endif

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
static void hexgrid_fillTile(int startX, int startY) {
    int y;

    Coordinate coordinates[HEXTILE_POINTS];

#ifdef HIRESBUILD
    WinSetCoordinateSystem(kCoordinatesDouble);
    hexgrid_tileCoords(startX * 2, startY * 2, coordinates, true);
    for (y = 0; y < HEXTILE_SIZE * 2; y++) {
        int actualY = y + startY * 2;
        int xOffset = hexgrid_tilePatternDouble[y];
        if (xOffset < 0) {
            continue;
        }
        drawhelper_drawLineBetweenCoordinates((Coordinate){startX * 2 + xOffset, actualY}, (Coordinate){startX * 2 + HEXTILE_SIZE * 2 - xOffset, actualY});
    }
    WinSetCoordinateSystem(kCoordinatesStandard);
#else
    WinHandle drawWindow = WinGetDrawWindow();
    int drawX = startX;
    int drawY = startY;
    if (hexgrid_drawTileFromCache(startX, startY, drawWindow, hexgrid_filledTileCacheWindow, hexgrid_filledTileCacheWindowColor)) {
        return;
    }
    if (hexgrid_createFilledTileCache(drawWindow)) {
        drawX = 0;
        drawY = 0;
    }

    hexgrid_tileCoords(drawX, drawY, coordinates, false);
    for (y = 0; y < HEXTILE_SIZE; y++) {
        int actualY = y + drawY;
        int xOffset = hexgrid_tilePattern[y];
        if (xOffset < 0) {
            continue;
        }
        drawhelper_drawLineBetweenCoordinates((Coordinate){drawX + xOffset, actualY}, (Coordinate){drawX + HEXTILE_SIZE - xOffset, actualY});
    }

    hexgrid_drawTileFromCache(startX, startY, drawWindow, hexgrid_filledTileCacheWindow, hexgrid_filledTileCacheWindowColor);
    WinSetDrawWindow(drawWindow);

#endif
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
void hexgrid_fillTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }
    hexgrid_fillTile(startPosition.x, startPosition.y);
}

HEXGRID_SECTION
void hexgrid_drawEntireGrid(Boolean adjustForViewport) {
    Coordinate screenSize = deviceinfo_screenSize();
    Coordinate center = viewport_convertedCoordinateInverted((Coordinate){screenSize.x / 2, screenSize.y / 2});
    Coordinate centerTile = hexgrid_tileAtPixel(center.x, center.y);
    Coordinate startPosition = hexgrid_tileStartPosition(centerTile.x, centerTile.y);
    WinHandle drawWindow = WinGetDrawWindow();
    int i, j;
    drawhelper_applyForeColor(ASBESTOS);
    hexgrid_drawTileAtPosition(centerTile, adjustForViewport);
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }

    for (i = 0; i < HEXGRID_COLS; i++) {
        for (j = 0; j < HEXGRID_ROWS; j++) {
            RectangleType rect;
            Coordinate targetPosition = hexgrid_tileStartPosition(i, j);
            if (i == centerTile.x && j == centerTile.y) {
                continue;
            }
            // copy the pattern
            if (adjustForViewport) {
                targetPosition = viewport_convertedCoordinate(targetPosition);
                if (targetPosition.x + HEXTILE_SIZE < 0 || targetPosition.y + HEXTILE_SIZE < 0 || targetPosition.x - HEXTILE_SIZE > screenSize.x || targetPosition.y - HEXTILE_SIZE > screenSize.y - BOTTOMMENU_HEIGHT) {
                    continue;
                }
            }
            RctSetRectangle(&rect, startPosition.x, startPosition.y, HEXTILE_SIZE + 1, HEXTILE_SIZE + 1);
            WinCopyRectangle(drawWindow, drawWindow, &rect, targetPosition.x, targetPosition.y, winPaint);
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
void hexgrid_drawSpriteAtTile(ImageSprite* imageSprite, Coordinate hexPosition) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate centerPosition = (Coordinate){startPosition.x + HEXTILE_SIZE / 2, startPosition.y + HEXTILE_SIZE / 2};
    drawhelper_drawSprite(imageSprite, viewport_convertedCoordinate(centerPosition));
}