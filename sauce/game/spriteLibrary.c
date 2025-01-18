#include "spriteLibrary.h"
#include "../graphicResources.h"
#include "../constants.h"
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
    if (spriteLibrary.initialized) {
        return;
    }

    spriteLibrary.shipSprite = (ImageSprite){
        GFX_RES_SHIPA_0,
        (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
    };
    spriteLibrary.shipSprite.imageData = drawhelper_loadImage(spriteLibrary.shipSprite.resourceId);
    
    spriteLibrary.initialized = true;
}