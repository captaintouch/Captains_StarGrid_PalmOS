#ifndef MINIMAP_H_
#define MINIMAP_H_
#include "models.h"

void minimap_draw(Pawn *pawns, int pawnCount, Coordinate drawPosition, Coordinate mapSize, Movement *activeMovement, Pawn *activePawn, Coordinate viewportOffset, Boolean colorSupport);
Coordinate minimap_viewportOffsetForTap(Coordinate tapPoint, Coordinate drawPosition, Coordinate mapSize);
#endif