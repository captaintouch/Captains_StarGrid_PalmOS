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


typedef struct SceneAnimation {
    Coordinate currentPosition;
    Line trajectory;
    Int32 launchTimestamp;
    ImageSprite *image;
} SceneAnimation;

typedef struct SpriteLibrary {
    Boolean initialized;
    ImageSprite shipOneSprite[GFX_FRAMECOUNT_SHIPA];
    ImageSprite shipTwoSprite[GFX_FRAMECOUNT_SHIPB];
    ImageSprite shipThreeSprite[GFX_FRAMECOUNT_SHIPC];
    ImageSprite shipFourSprite[GFX_FRAMECOUNT_SHIPD];
    ImageSprite baseSprite;
    ImageSprite torpedoAnimation[GFX_FRAMECOUNT_TORP];
    ImageSprite explosionAnimation[GFX_FRAMECOUNT_EXPLOSION];
    ImageSprite healthSprite;
    ImageSprite humanSprite;
    ImageSprite cpuSprite;
    ImageSprite ufoSprite;
    ImageSprite cometSprite;
} SpriteLibrary;

SpriteLibrary spriteLibrary;

void spriteLibrary_initialize();
void spriteLibrary_clean();
ImageSprite imageSprite(int resourceId, Coordinate size);
ImageSprite *spriteLibrary_factionShipSprite(int faction);

#endif
