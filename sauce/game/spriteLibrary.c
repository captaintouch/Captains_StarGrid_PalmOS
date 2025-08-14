#include "spriteLibrary.h"

#include "../constants.h"
#include "../graphicResources.h"
#include "drawhelper.h"

static ImageData *drawhelper_loadImage(UInt16 bitmapId) {
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

    spriteLibrary.humanSprite = (ImageSprite){
        GFX_RES_HUMAN,
        (Coordinate){16, 16},
    };
    spriteLibrary.humanSprite.imageData =
        drawhelper_loadImage(spriteLibrary.humanSprite.resourceId);

    spriteLibrary.ufoSprite = (ImageSprite){
        GFX_RES_UFO,
        (Coordinate){16, 16},
    };
    spriteLibrary.ufoSprite.imageData =
        drawhelper_loadImage(spriteLibrary.ufoSprite.resourceId);

    spriteLibrary.tileFeaturedSprite = (ImageSprite){
        GFX_RES_TILEFEATURED,
        (Coordinate){20, 20},
    };
    spriteLibrary.tileFeaturedSprite.imageData =
        drawhelper_loadImage(spriteLibrary.tileFeaturedSprite.resourceId);

    spriteLibrary.tileWarnSprite = (ImageSprite){
        GFX_RES_TILEWARN,
        (Coordinate){20, 20},
    };
    spriteLibrary.tileWarnSprite.imageData =
        drawhelper_loadImage(spriteLibrary.tileWarnSprite.resourceId);

    spriteLibrary.tileMoveSprite = (ImageSprite){
        GFX_RES_TILEMOVE,
        (Coordinate){20, 20},
    };
    spriteLibrary.tileMoveSprite.imageData =
        drawhelper_loadImage(spriteLibrary.tileMoveSprite.resourceId);

    spriteLibrary.tileAttackSprite = (ImageSprite){
        GFX_RES_TILEATTACK,
        (Coordinate){20, 20},
    };
    spriteLibrary.tileAttackSprite.imageData =
        drawhelper_loadImage(spriteLibrary.tileAttackSprite.resourceId);

    spriteLibrary.cometSprite = (ImageSprite){
        GFX_RES_COMET,
        (Coordinate){16, 16},
    };
    spriteLibrary.cometSprite.imageData =
        drawhelper_loadImage(spriteLibrary.cometSprite.resourceId);


    spriteLibrary.cpuSprite = (ImageSprite){
        GFX_RES_CPUPLAYER,
        (Coordinate){16, 16},
    };
    spriteLibrary.cpuSprite.imageData =
        drawhelper_loadImage(spriteLibrary.cpuSprite.resourceId);

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

    for (i = 0; i < GFX_FRAMECOUNT_STARANIM; i++) {
        spriteLibrary.starAnimation[i] = (ImageSprite){
            GFX_RES_STARANIM_0 + i,
            (Coordinate){19, 19},
        };
        spriteLibrary.starAnimation[i].imageData =
            drawhelper_loadImage(spriteLibrary.starAnimation[i].resourceId);
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
    for (i = 0; i < GFX_FRAMECOUNT_STARANIM; i++) {
        drawhelper_releaseImage(spriteLibrary.starAnimation[i].imageData);
    }
    drawhelper_releaseImage(spriteLibrary.baseSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.healthSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.humanSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.ufoSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.cometSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.tileAttackSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.tileMoveSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.tileWarnSprite.imageData);
    drawhelper_releaseImage(spriteLibrary.tileFeaturedSprite.imageData);
    spriteLibrary.initialized = false;
}

ImageSprite spriteLibrary_nebulaSprite() {

    ImageSprite nebulaSprite = (ImageSprite){
        GFX_RES_NEBULA,
        (Coordinate){16, 16},
    };
    nebulaSprite.imageData = drawhelper_loadImage(GFX_RES_NEBULA);
    return nebulaSprite;
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
