#include "../i_resource.h"

#include <stdlib.h>

#include "../../constants.h"
#include "../../graphicResources.h"
#include "web_bitmap.h"
#include "web_assets_generated.h"

/* ---- Bitmaps -------------------------------------------------------------
   The Palm OS backend (i_resource_palmos.c) resolves a bitmap by looking up
   an exact resource id via DmGetResource(bitmapRsc, bitmapId) against
   entries declared in resources/ui.rcp + the generated
   resources/graphicResources.rcp (built from resources/graphicResources.map
   by generateResourceFile.sh, with pixel data produced by
   generateBitmaps.sh out of the real art in resources/ships,
   resources/others, resources/others_noresize).

   This backend mirrors the same per-id lookup against web_assetTable, which
   is the wasm-side equivalent: generate_web_assets.py bakes the very same
   source PNGs (rotating each ship into its 8 frames, masking the tile
   hexagons) into web_assets_generated.h, keyed by the very same
   GFX_RES_* ids from graphicResources.map. Re-run that script (or
   `make -f Makefile.web assets`) after touching any source asset. ---- */

static WebBitmap bitmapForId(unsigned short id) {
    unsigned int i;
    for (i = 0; i < sizeof(web_assetTable) / sizeof(web_assetTable[0]); i++) {
        if (web_assetTable[i].id == id) {
            WebBitmap bitmap;
            bitmap.width = web_assetTable[i].width;
            bitmap.height = web_assetTable[i].height;
            bitmap.color = web_assetTable[i].color;
            bitmap.alpha = web_assetTable[i].alpha;
            bitmap.resourceId = id;
            return bitmap;
        }
    }
    /* Unknown id: same "missing resource" outcome DmGetResource() would
       produce on Palm OS for an undeclared id - no asset to draw. */
    {
        WebBitmap empty;
        empty.width = 0;
        empty.height = 0;
        empty.color = NULL;
        empty.alpha = NULL;
        empty.resourceId = id;
        return empty;
    }
}

IBitmapHandle *iresource_loadBitmap(unsigned short bitmapId, void **outBitmapPtr) {
    WebBitmap *bitmap = (WebBitmap *)malloc(sizeof(WebBitmap));
    if (bitmap == NULL) {
        if (outBitmapPtr != NULL) {
            *outBitmapPtr = NULL;
        }
        return NULL;
    }
    *bitmap = bitmapForId(bitmapId);
    if (outBitmapPtr != NULL) {
        *outBitmapPtr = (void *)bitmap;
    }
    return (IBitmapHandle *)bitmap;
}

void iresource_releaseBitmap(IBitmapHandle *handle) {
    free(handle);
}

/* ---- Strings: small id -> text table mirroring resources/ui.rcp, shared
   verbatim with the CLI backend's table. ---- */

typedef struct StringEntry {
    unsigned short id;
    char *text;
} StringEntry;

static StringEntry web_strings[] = {
    {STRING_MOVE, "Move"},
    {STRING_PHASER, "Phaser"},
    {STRING_TORPEDO, "Torpedo"},
    {STRING_SHOCKWAVE, "Shockwave"},
    {STRING_BUILDSHIP, "Build ship"},
    {STRING_HEALTHPACK, "Health pack"},
    {STRING_TORPEDOPACK, "Torpedo pack"},
    {STRING_WARP, "Warp home"},
    {STRING_CANCEL, "Cancel"},
    {STRING_MOVING, "Moving"},
    {STRING_ATTACKING, "Attacking"},
    {STRING_NOACTION, "No action"},
    {STRING_NEXT, "next"},
    {STRING_ENDTURN, "end turn"},
    {STRING_NEW, "NEW"},
    {STRING_RANK, "RANK"},
    {STRING_ABOUT, "ABOUT"},
    {STRING_CAPTAINS, "Captain's"},
    {STRING_STARGRID, "StarGrid"},
    {STRING_PLAYERCPU, "Player/CPU"},
    {STRING_CONFIG, "Configuration"},
    {STRING_PLAYERS, "# Players:"},
    {STRING_LEVELSCORE, "Level Score"},
    {STRING_HOWTOPLAY, "Capture all enemy flags or destroy all enemy ships. Good luck, Captain!"},
    {STRING_DESTROYED, "Destroyed:"},
    {STRING_CAPTURED, "Captured:"},
    {STRING_FLAGSSTOLEN, "Flags stolen"},
    {STRING_SHIPSLOST, "Ships lost"},
    {STRING_SHIPSDESTROYED, "Ships/bases destroyed"},
    {STRING_SHIPSCAPTURED, "Ships captured"},
    {STRING_FLAGSCAPTURED, "Flags captured"},
    {STRING_UNTILNEXTRANK, "p. needed to advance"},
    {STRING_RANK0, "Star Cadet"},
    {STRING_RANK1, "Comms Technician"},
    {STRING_RANK2, "Cosmic Trooper"},
    {STRING_RANK3, "Orbital Specialist"},
    {STRING_RANK4, "Warp Sergeant"},
    {STRING_RANK5, "Astro Chief"},
    {STRING_RANK6, "Nova Ensign"},
    {STRING_RANK7, "Lieutenant"},
    {STRING_RANK8, "Commander"},
    {STRING_RANK9, "Captain Eclipse"},
    {STRING_RANK10, "Commodore Quasar"},
    {STRING_RANK11, "Vice Admiral"},
    {STRING_RANK12, "Admiral"},
    {STRING_RANK13, "Fleet Marshal"},
    {STRING_RANK14, "Rift Warden"},
    {STRING_RANK15, "High Executor"},
    {STRING_RANK16, "Star Archon"},
    {STRING_RANK17, "Celestial Strategist"},
    {STRING_RANK18, "Grid Overseer"},
};

static char web_emptyString[] = "";

char *iresource_loadString(unsigned short stringId, void **outHandle) {
    unsigned int i;
    if (outHandle != NULL) {
        *outHandle = NULL; /* static storage, nothing to release */
    }
    for (i = 0; i < sizeof(web_strings) / sizeof(web_strings[0]); i++) {
        if (web_strings[i].id == stringId) {
            return web_strings[i].text;
        }
    }
    return web_emptyString;
}

void iresource_releaseString(void *handle) {
    (void)handle;
}

static char web_version[] = "WEB";

char *iresource_loadAppVersion(void **outHandle) {
    if (outHandle != NULL) {
        *outHandle = NULL;
    }
    return web_version;
}

void iresource_releaseAppVersion(void *handle) {
    (void)handle;
}
