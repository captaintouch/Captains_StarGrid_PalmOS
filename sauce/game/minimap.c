#include "minimap.h"
#include "hexgrid.h"
#include "drawhelper.h"
#include "../deviceinfo.h"
#include "../constants.h"
#include "pawn.h"

static Coordinate minimap_positionOnMap(Coordinate coordinate, Coordinate mapSize, Coordinate gridSize) {
    return (Coordinate){(float)coordinate.x / (float)gridSize.x * (float)mapSize.x, (float)coordinate.y / (float)gridSize.y * (float)mapSize.y};
}

static void minimap_updateViewport(Coordinate originalOffset, Coordinate mapSize, Coordinate gridSize, Coordinate *targetOffset, Coordinate *targetSize) {
    Coordinate screenSize = deviceinfo_screenSize();
    Coordinate offset = minimap_positionOnMap(originalOffset, mapSize, gridSize);
    targetOffset->x = offset.x;
    targetOffset->y = offset.y;
    targetSize->x = (float)screenSize.x / (float)gridSize.x * (float)mapSize.x;
    targetSize->y = ((float)screenSize.y - (float)BOTTOMMENU_HEIGHT) / (float)gridSize.y * (float)mapSize.y;
}

void minimap_draw(Pawn *pawns, int pawnCount, Coordinate drawPosition, Coordinate mapSize, Movement *activeMovement, Pawn *activePawn, Coordinate viewportOffset) {
    int i, j, k;
    RectangleType rect = (RectangleType){drawPosition.x, drawPosition.y, mapSize.x, mapSize.y};
    Coordinate gridSize = hexgrid_size();
    Coordinate minimapViewportOffset;
    Coordinate minimapViewportSize;
    RectangleType minimapViewportRect;
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect, 0);
    for (i = 0; i < pawnCount; i++) {
        Coordinate convertedPoint;
        if (activeMovement != NULL && activeMovement->pawn == &pawns[i]) {
            convertedPoint = minimap_positionOnMap(activeMovement->pawnPosition, mapSize, gridSize);
        } else {
            convertedPoint = minimap_positionOnMap(hexgrid_tileCenterPosition(pawns[i].position), mapSize, gridSize);
        }
        convertedPoint.x += drawPosition.x;
        convertedPoint.y += drawPosition.y;
        drawhelper_applyForeColor(pawn_factionColor(pawns[i].faction));

        for (j = -1; j <= 1; j++) {
            for (k = -1; k <= 1; k++) {
                drawhelper_drawPoint((Coordinate){convertedPoint.x + j, convertedPoint.y + k});
            }
        }

        if (&pawns[i] == activePawn) {
            drawhelper_applyForeColor(CLOUDS);
            drawhelper_drawBoxAround(convertedPoint, 3);
        }

    }

    // Draw the viewport with a border around it
    drawhelper_applyForeColor(CLOUDS);
    minimap_updateViewport(viewportOffset, mapSize, gridSize, &minimapViewportOffset, &minimapViewportSize);
    minimapViewportRect = (RectangleType){minimapViewportOffset.x + drawPosition.x, minimapViewportOffset.y + drawPosition.y, minimapViewportSize.x, minimapViewportSize.y};
    drawhelper_borderRectangle(&minimapViewportRect);
}

Coordinate minimap_viewportOffsetForTap(Coordinate tapPoint, Coordinate drawPosition, Coordinate mapSize) {
    Coordinate screenSize = deviceinfo_screenSize();
    Coordinate deltaCoordinate = (Coordinate){tapPoint.x - drawPosition.x, tapPoint.y - drawPosition.y};
    if (deltaCoordinate.x < 0 || deltaCoordinate.x > mapSize.x || deltaCoordinate.y < 0 || deltaCoordinate.y > mapSize.y ) {
        return (Coordinate){-1, -1};
    }
    return (Coordinate){ (float)deltaCoordinate.x / (float)mapSize.x * (float)screenSize.x * 2, (float)deltaCoordinate.y / (float)mapSize.y * (float)screenSize.y * 2};
}