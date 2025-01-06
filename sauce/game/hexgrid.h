#ifndef HEXGRID_H_
#define HEXGRID_H_

#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"

void hexgrid_initialize();
Coordinate hexgrid_tileAtPixel(int x, int y);
void hexgrid_drawEntireGrid();
void hexgrid_drawTileAtPosition(Coordinate hexPosition);
void hexgrid_fillTileAtPosition(Coordinate hexPosition);
void hexgrid_drawSpriteAtTile(ImageSprite *imageSprite, Coordinate hexPosition);
#endif