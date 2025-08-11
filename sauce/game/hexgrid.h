#ifndef HEXGRID_H_
#define HEXGRID_H_

#define HEXGRID_SECTION  __attribute__ ((section ("hexgrid")))

#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"
 
void hexgrid_initialize() HEXGRID_SECTION;
void hexgrid_cleanup() HEXGRID_SECTION;
Coordinate hexgrid_tileAtPixel(int x, int y) HEXGRID_SECTION;
Coordinate hexgrid_tileCenterPosition(Coordinate tilePosition) HEXGRID_SECTION;
void hexgrid_drawEntireGrid(Boolean adjustForViewport) HEXGRID_SECTION;
void hexgrid_drawTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport) HEXGRID_SECTION;
void hexgrid_fillTileAtPosition(Coordinate hexPosition, Boolean adjustForViewport, FilledTileType tileType) HEXGRID_SECTION; 
void hexgrid_drawSpriteAtTile(ImageSprite* imageSprite, Coordinate hexPosition, Boolean adjustForViewport) HEXGRID_SECTION; 
Coordinate hexgrid_size() HEXGRID_SECTION;
#endif
