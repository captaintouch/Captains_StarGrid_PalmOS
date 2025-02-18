#ifndef SPRITELIBRARY_H_
#define SPRITELIBRARY_H_
#include <PalmOS.h>
#include "models.h"
#include "../graphicResources.h"

typedef struct ImageData {
    MemHandle resource;
    BitmapPtr bitmapPtr;
} ImageData;

typedef struct ImageSprite {
    int resourceId;
    Coordinate size;
    ImageData *imageData;
} ImageSprite;

typedef struct SpriteLibrary {
    Boolean initialized;
    ImageSprite shipSprite[GFX_FRAMECOUNT_SHIPA];
    ImageSprite shipCloakedSprite[GFX_FRAMECOUNT_SHIPCLOAKED];
    ImageSprite baseSprite;
    ImageSprite torpedoAnimation[GFX_FRAMECOUNT_TORP];
} SpriteLibrary;

SpriteLibrary spriteLibrary;

void spriteLibrary_initialize();
ImageSprite imageSprite(int resourceId, Coordinate size);

#endif