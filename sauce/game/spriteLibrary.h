#ifndef SPRITELIBRARY_H_
#define SPRITELIBRARY_H_
#include <PalmOS.h>
#include "models.h"

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
    ImageSprite shipSprite[8];
    ImageSprite shipCloakedSprite[8];
    ImageSprite baseSprite;
    ImageSprite torpedoAnimation[6];
} SpriteLibrary;

SpriteLibrary spriteLibrary;

void spriteLibrary_initialize();
ImageSprite imageSprite(int resourceId, Coordinate size);

#endif