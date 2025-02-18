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
    spriteLibrary.shipSprite[i] = (ImageSprite){
        GFX_RES_SHIPA_0 + i,
        (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
    };
    spriteLibrary.shipSprite[i].imageData =
        drawhelper_loadImage(spriteLibrary.shipSprite[i].resourceId);
  }

  for (i = 0; i < GFX_FRAMECOUNT_SHIPCLOAKED; i++) {
    spriteLibrary.shipCloakedSprite[i] = (ImageSprite){
        GFX_RES_SHIPCLOAKED_0 + i,
        (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
    };
    spriteLibrary.shipCloakedSprite[i].imageData =
        drawhelper_loadImage(spriteLibrary.shipCloakedSprite[i].resourceId);
  }

  spriteLibrary.baseSprite = (ImageSprite){
      GFX_RES_BASE,
      (Coordinate){HEXTILE_PAWNSIZE, HEXTILE_PAWNSIZE},
  };
  spriteLibrary.baseSprite.imageData =
      drawhelper_loadImage(spriteLibrary.baseSprite.resourceId);

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