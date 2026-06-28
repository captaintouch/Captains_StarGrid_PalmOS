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

/* The tile geometry is a runtime value so the grid can scale (zoom) to the
   screen. It defaults to the historical fixed size, and platforms that don't
   opt into zooming keep exactly that size, so their rendering is unchanged. */
static int hexgrid_currentTileSize = HEXTILE_SIZE;
static int hexgrid_currentSegmentSize = HEXTILE_SEGMENT_SIZE;

int hexgrid_tilePattern[HEXTILE_MAXSIZE];

HEXGRID_SECTION
void hexgrid_cleanup() {
}

HEXGRID_SECTION
int hexgrid_tileSize() {
    return hexgrid_currentTileSize;
}

HEXGRID_SECTION
int hexgrid_pawnSize() {
    return hexgrid_currentTileSize * HEXTILE_PAWNSIZE / HEXTILE_SIZE;
}

HEXGRID_SECTION
static void hexgrid_tileCoords(int startX, int startY, Coordinate coordinates[]) {
    int hexTileSize = hexgrid_currentTileSize;
    int hexTileSegmentSize = hexgrid_currentSegmentSize;
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
    hexgrid_tileCoords(startX, startY, coordinates);

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
static void hexgrid_rebuildPattern() {
    int x, y;
    Coordinate coordinates[HEXTILE_POINTS];
    hexgrid_tileCoords(0, 0, coordinates);
    for (y = 0; y < hexgrid_currentTileSize; y++) {
        hexgrid_tilePattern[y] = hexgrid_currentTileSize / 2;
        for (x = 0; x < hexgrid_currentTileSize; x++) {
            if (hexgrid_isInsideTile(coordinates, (Coordinate){x, y})) {
                hexgrid_tilePattern[y] = x;
                break;
            }
        }
    }
}

HEXGRID_SECTION
void hexgrid_setTileSize(int tileSize) {
    if (tileSize < HEXTILE_MINSIZE) {
        tileSize = HEXTILE_MINSIZE;
    }
    if (tileSize > HEXTILE_MAXSIZE) {
        tileSize = HEXTILE_MAXSIZE;
    }
    hexgrid_currentTileSize = tileSize;
    hexgrid_currentSegmentSize = tileSize * HEXTILE_SEGMENT_SIZE / HEXTILE_SIZE;
    /* Keep sprite scaling in lock-step with the tile size (100% at the default
       size, so the unscaled draw path is used and nothing changes for the
       historical platforms). */
    drawhelper_setSpriteScale(tileSize * 100 / HEXTILE_SIZE);
    hexgrid_rebuildPattern();
}

HEXGRID_SECTION
static int hexgrid_desiredTileSize() {
    int desired = deviceinfo_gridTileSize(deviceinfo_screenSize());
    if (desired <= 0) {
        desired = HEXTILE_SIZE;
    }
    if (desired < HEXTILE_MINSIZE) {
        desired = HEXTILE_MINSIZE;
    }
    if (desired > HEXTILE_MAXSIZE) {
        desired = HEXTILE_MAXSIZE;
    }
    return desired;
}

HEXGRID_SECTION
void hexgrid_initialize() {
    hexgrid_setTileSize(hexgrid_desiredTileSize());
}

HEXGRID_SECTION
void hexgrid_rescaleIfNeeded() {
    int desired = hexgrid_desiredTileSize();
    if (desired != hexgrid_currentTileSize) {
        hexgrid_setTileSize(desired);
    }
}

HEXGRID_SECTION
static Coordinate hexgrid_tileStartPosition(int column, int row) {
    if (row % 2 != 0) {
        return (Coordinate){column * hexgrid_currentTileSize, row * hexgrid_currentTileSize - (row * hexgrid_currentSegmentSize)};
    } else {
        return (Coordinate){column * hexgrid_currentTileSize + hexgrid_currentTileSize / 2, row * hexgrid_currentTileSize - hexgrid_currentSegmentSize - ((row - 1) * hexgrid_currentSegmentSize)};
    }
}

HEXGRID_SECTION
Coordinate hexgrid_tileCenterPosition(Coordinate tilePosition) {
    Coordinate position = hexgrid_tileStartPosition(tilePosition.x, tilePosition.y);
    return (Coordinate){position.x + hexgrid_currentTileSize / 2, position.y + hexgrid_currentTileSize / 2};
}

HEXGRID_SECTION
void hexgrid_drawTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }
    hexgrid_drawTile(startPosition.x, startPosition.y);
}

/* Badges the letter-hex buttons (NEW/RANK/ABOUT) as raised chrome rather than
   plain outlined tiles: a light highlight along the two upper edges, a dark
   shadow along the two lower edges, mirroring the bevel treatment already
   used for the title bar and bottom menu. */
HEXGRID_SECTION
void hexgrid_drawTileBevelAtPosition(Coordinate hexPosition, Boolean adjustForViewport, AppColor highlightColor, AppColor shadowColor) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate coordinates[HEXTILE_POINTS];
    if (adjustForViewport) {
        startPosition = viewport_convertedCoordinate(startPosition);
    }
    hexgrid_tileCoords(startPosition.x, startPosition.y, coordinates);

    drawhelper_applyForeColor(highlightColor);
    drawhelper_drawLineBetweenCoordinates(coordinates[5], coordinates[0]);
    drawhelper_drawLineBetweenCoordinates(coordinates[0], coordinates[1]);

    drawhelper_applyForeColor(shadowColor);
    drawhelper_drawLineBetweenCoordinates(coordinates[2], coordinates[3]);
    drawhelper_drawLineBetweenCoordinates(coordinates[3], coordinates[4]);
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
                if (targetPosition.x + hexgrid_currentTileSize < 0 || targetPosition.y + hexgrid_currentTileSize < 0 || targetPosition.x - hexgrid_currentTileSize > screenSize.x || targetPosition.y - hexgrid_currentTileSize > screenSize.y - BOTTOMMENU_HEIGHT) {
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
        offset += hexgrid_currentTileSize - hexgrid_currentSegmentSize;
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
            if (yIndex < 0 || yIndex >= hexgrid_currentTileSize) {
                continue;
            }
            if (x >= tileCoordinate.x + hexgrid_tilePattern[yIndex] && x <= tileCoordinate.x + hexgrid_currentTileSize - hexgrid_tilePattern[yIndex]) {
                return (Coordinate){c, r};
            }
        }
    }
    return (Coordinate){-1, -1};
}

HEXGRID_SECTION
Coordinate hexgrid_size() {
    Coordinate lastPosition = hexgrid_tileCenterPosition((Coordinate){HEXGRID_COLS - 1, HEXGRID_ROWS - 1});
    return (Coordinate){lastPosition.x + hexgrid_currentTileSize + 5, lastPosition.y + hexgrid_currentTileSize + 5};
}

HEXGRID_SECTION
void hexgrid_drawSpriteAtTile(ImageSprite* imageSprite, Coordinate hexPosition, Boolean adjustForViewport) {
    Coordinate startPosition = hexgrid_tileStartPosition(hexPosition.x, hexPosition.y);
    Coordinate centerPosition = (Coordinate){startPosition.x + hexgrid_currentTileSize / 2, startPosition.y + hexgrid_currentTileSize / 2};
    drawhelper_drawSprite(imageSprite, adjustForViewport ? viewport_convertedCoordinate(centerPosition) : centerPosition);
}
