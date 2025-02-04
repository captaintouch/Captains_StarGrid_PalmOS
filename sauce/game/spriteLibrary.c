#include "spriteLibrary.h"

#include "../constants.h"
#include "../graphicResources.h"
#include "drawhelper.h"

ImageData *drawhelper_loadImage(UInt16 bitmapId) {
    MemHandle bitmapH;

    bitmapH = DmGetResource(bitmapRsc, bitmapId);
    if (bitmapH) {
        BitmapPtr bitmap = (BitmapPtr)MemHandleLock(bitmapH);
        ImageData *imageData = (ImageData *)MemPtrNew(sizeof(ImageData));
        MemSet(imageData, sizeof(ImageData), 0);
        imageData->bitmapPtr = bitmap;
        imageData->resource = bitmapH;
        return imageData;
    }
    return NULL;
}

void spriteLibrary_initialize() {
    int i;
    if (spriteLibrary.initialized) {
        return;
    }
    for (i = 0; i < 8; i++) {
        spriteLibrary.shipSprite[i] = (ImageSprite){
            GFX_RES_SHIPA_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipSprite[i].imageData = drawhelper_loadImage(spriteLibrary.shipSprite[i].resourceId);
    }

    for (i = 0; i < 8; i++) {
        spriteLibrary.shipCloakedSprite[i] = (ImageSprite){
            GFX_RES_SHIPCLOAKED_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipCloakedSprite[i].imageData = drawhelper_loadImage(spriteLibrary.shipCloakedSprite[i].resourceId);
    }
    
    spriteLibrary.flagSprite = (ImageSprite){
            GFX_RES_FLAG,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.flagSprite.imageData = drawhelper_loadImage(spriteLibrary.flagSprite.resourceId);

    spriteLibrary.initialized = true;
}