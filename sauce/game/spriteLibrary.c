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
    for (i = 0; i < GFX_FRAMECOUNT_SHIPA; i++) {
        spriteLibrary.shipOneSprite[i] = (ImageSprite){
            GFX_RES_SHIPA_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipOneSprite[i].imageData =
            drawhelper_loadImage(spriteLibrary.shipOneSprite[i].resourceId);
    }

    for (i = 0; i < GFX_FRAMECOUNT_SHIPB; i++) {
        spriteLibrary.shipTwoSprite[i] = (ImageSprite){
            GFX_RES_SHIPB_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipTwoSprite[i].imageData =
            drawhelper_loadImage(spriteLibrary.shipTwoSprite[i].resourceId);
    }

    for (i = 0; i < GFX_FRAMECOUNT_SHIPC; i++) {
        spriteLibrary.shipThreeSprite[i] = (ImageSprite){
            GFX_RES_SHIPC_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipThreeSprite[i].imageData =
            drawhelper_loadImage(spriteLibrary.shipThreeSprite[i].resourceId);
    }

    for (i = 0; i < GFX_FRAMECOUNT_SHIPD; i++) {
        spriteLibrary.shipFourSprite[i] = (ImageSprite){
            GFX_RES_SHIPD_0 + i,
            (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
        };
        spriteLibrary.shipFourSprite[i].imageData =
            drawhelper_loadImage(spriteLibrary.shipFourSprite[i].resourceId);
    }

    spriteLibrary.baseSprite = (ImageSprite){
        GFX_RES_BASE,
        (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
    };
    spriteLibrary.baseSprite.imageData =
        drawhelper_loadImage(spriteLibrary.baseSprite.resourceId);

    spriteLibrary.healthSprite = (ImageSprite){
        GFX_RES_HEALTHBW,
        (Coordinate){8, 8},
    };
    spriteLibrary.healthSprite.imageData =
        drawhelper_loadImage(spriteLibrary.healthSprite.resourceId);

    for (i = 0; i < GFX_FRAMECOUNT_TORP; i++) {
        spriteLibrary.torpedoAnimation[i] = (ImageSprite){
            GFX_RES_TORP_0 + i,
            (Coordinate){10, 10},
        };
        spriteLibrary.torpedoAnimation[i].imageData =
            drawhelper_loadImage(spriteLibrary.torpedoAnimation[i].resourceId);
    }

    for (i = 0; i < GFX_FRAMECOUNT_EXPLOSION; i++) {
        spriteLibrary.explosionAnimation[i] = (ImageSprite){
            GFX_RES_EXPLOSION_0 + i,
            (Coordinate){19, 19},
        };
        spriteLibrary.explosionAnimation[i].imageData =
            drawhelper_loadImage(spriteLibrary.explosionAnimation[i].resourceId);
    }

    spriteLibrary.initialized = true;
}

void spriteLibrary_clean() {
    int i;
    for (i = 0; i < GFX_FRAMECOUNT_SHIPA; i++) {
        drawhelper_releaseImage(spriteLibrary.shipOneSprite[i].imageData);
    }
    for (i = 0; i < GFX_FRAMECOUNT_SHIPB; i++) {
        drawhelper_releaseImage(spriteLibrary.shipTwoSprite[i].imageData);
    }
    for (i = 0; i < GFX_FRAMECOUNT_SHIPC; i++) {
        drawhelper_releaseImage(spriteLibrary.shipThreeSprite[i].imageData);
    }
    for (i = 0; i < GFX_FRAMECOUNT_SHIPD; i++) {
        drawhelper_releaseImage(spriteLibrary.shipFourSprite[i].imageData);
    }
    for (i = 0; i < GFX_FRAMECOUNT_TORP; i++) {
        drawhelper_releaseImage(spriteLibrary.torpedoAnimation[i].imageData);
    }
    for (i = 0; i < GFX_FRAMECOUNT_EXPLOSION; i++) {
        drawhelper_releaseImage(spriteLibrary.explosionAnimation[i].imageData);
    }
    drawhelper_releaseImage(spriteLibrary.baseSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.healthSprite.imageData);
    spriteLibrary.initialized = false;
}

ImageSprite *spriteLibrary_factionShipSprite(int faction) {
    switch (faction % 4) {
        case 0:
            return spriteLibrary.shipOneSprite;
        case 1:
            return spriteLibrary.shipTwoSprite;
        case 2:
            return spriteLibrary.shipThreeSprite;
        case 3:
            return spriteLibrary.shipFourSprite;
    }
    return spriteLibrary.shipOneSprite;
}