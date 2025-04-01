#ifndef HEXGRID_H_
#define HEXGRID_H_

#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"

void hexgrid_initialize();
Coordinate hexgrid_tileAtPixel(int x, int y);
Coordinate hexgrid_tileCenterPosition(Coordinate tilePosition);
void hexgrid_drawEntireGrid(Boolean adjustForViewport);
void hexgrid_drawTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport);
void hexgrid_fillTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport);
void hexgrid_drawSpriteAtTile(ImageSprite *imageSprite, Coordinate hexPosition);
Coordinate hexgrid_size();
#endif