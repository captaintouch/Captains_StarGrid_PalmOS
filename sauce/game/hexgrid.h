#ifndef HEXGRID_H_
#define HEXGRID_H_

#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"

Coordinate hexgrid_tileAtPixel(float x, float y);
void hexgrid_drawEntireGrid();
void hexgrid_drawTileAtPosition(Coordinate hexPosition);
void hexgrid_fillTileAtPosition(Coordinate hexPosition, AppColor color, WinHandle buffer);
void hexgrid_drawSpriteAtTile(ImageSprite *imageSprite, Coordinate hexPosition);
#endif