#include "minimap.h"
#include "hexgrid.h"
#include "drawhelper.h"
#include "../deviceinfo.h"

static Coordinate minimap_positionOnMap(Coordinate coordinate, Coordinate mapSize, Coordinate gridSize) {
    return (Coordinate){(float)coordinate.x / (float)gridSize.x * (float)mapSize.x, (float)coordinate.y / (float)gridSize.y * (float)mapSize.y};
}

void minimap_draw(Pawn *pawns, int pawnCount, Coordinate drawPosition, Coordinate mapSize, Movement *activeMovement, Pawn *activePawn) {
    int i, j, k;
    RectangleType rect = (RectangleType){drawPosition.x, drawPosition.y, mapSize.x, mapSize.y};
    Coordinate gridSize = hexgrid_size();
    drawhelper_applyForeColor(DRACULAORCHID);
    drawhelper_fillRectangle(&rect);
    for (i = 0; i < pawnCount; i++) {
        Coordinate convertedPoint;
        if (activeMovement != NULL && activeMovement->pawn == &pawns[i]) {
            convertedPoint = minimap_positionOnMap(activeMovement->pawnPosition, mapSize, gridSize);
        } else {
            convertedPoint = minimap_positionOnMap(hexgrid_tileCenterPosition(pawns[i].position), mapSize, gridSize);
        }
        convertedPoint.x += drawPosition.x;
        convertedPoint.y += drawPosition.y;
        drawhelper_applyForeColor(EMERALD);  // TODO: get color for faction

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
}
