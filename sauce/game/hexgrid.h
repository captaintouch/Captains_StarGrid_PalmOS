#ifndef HEXGRID_H_
#define HEXGRID_H_

#include "models.h"

Coordinate hexgrid_tileAtPixel(float x, float y);
void hexgrid_drawEntireGrid();
void hexgrid_drawTileAtPosition(Coordinate hexPosition);
#endif