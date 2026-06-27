#include "../i_resource.h"

#include <stdlib.h>

#include "../../constants.h"
#include "../../graphicResources.h"
#include "web_bitmap.h"

/* ---- Bitmaps -------------------------------------------------------------
   The Palm OS backend (i_resource_palmos.c) resolves a bitmap by looking up
   an exact resource id via DmGetResource(bitmapRsc, bitmapId) against
   entries declared one-by-one in resources/ui.rcp. This repo currently has
   no .rcp BITMAP declarations (and no .bmp files) for any GFX_RES_SHIP,
   BASE, TORP, EXPLOSION, etc id, so there is no real pixel data to port
   from for either platform yet.

   To still mirror that per-id resource model rather than the coarse
   per-faction id *range* this table used to key off, every individual
   GFX_RES_* id is listed explicitly below with its own stub entry. Each row
   is the placeholder "asset" for that exact resource id, the same way a
   real .rcp BITMAP entry would be; swapping in real higher-res art later is
   just a matter of pointing a row at decoded image data instead of a flat
   color, without touching the lookup mechanism. ---- */

#define RGB565(r, g, b) (((unsigned short)((r) & 0x1F) << 11) | ((unsigned short)((g) & 0x3F) << 5) | (unsigned short)((b) & 0x1F))

typedef struct BitmapResEntry {
    unsigned short id;
    WebBitmap bitmap;
} BitmapResEntry;

/* Per-frame shade step within a sprite sheet, so individual frames (e.g.
   each ship rotation / explosion frame) remain visually distinct from one
   another instead of collapsing into one flat per-faction color. */
#define SHADE(base, step) ((unsigned char)((base) + (step) * 3 > 31 ? 31 : (base) + (step) * 3))

static BitmapResEntry web_bitmapTable[] = {
    {GFX_RES_SHIPA_0, {RGB565(SHADE(8, 0), 0, 0), 0}}, {GFX_RES_SHIPA_1, {RGB565(SHADE(8, 1), 0, 0), 0}},
    {GFX_RES_SHIPA_2, {RGB565(SHADE(8, 2), 0, 0), 0}}, {GFX_RES_SHIPA_3, {RGB565(SHADE(8, 3), 0, 0), 0}},
    {GFX_RES_SHIPA_4, {RGB565(SHADE(8, 4), 0, 0), 0}}, {GFX_RES_SHIPA_5, {RGB565(SHADE(8, 5), 0, 0), 0}},
    {GFX_RES_SHIPA_6, {RGB565(SHADE(8, 6), 0, 0), 0}}, {GFX_RES_SHIPA_7, {RGB565(SHADE(8, 7), 0, 0), 0}},

    {GFX_RES_SHIPB_0, {RGB565(0, 0, SHADE(8, 0)), 0}}, {GFX_RES_SHIPB_1, {RGB565(0, 0, SHADE(8, 1)), 0}},
    {GFX_RES_SHIPB_2, {RGB565(0, 0, SHADE(8, 2)), 0}}, {GFX_RES_SHIPB_3, {RGB565(0, 0, SHADE(8, 3)), 0}},
    {GFX_RES_SHIPB_4, {RGB565(0, 0, SHADE(8, 4)), 0}}, {GFX_RES_SHIPB_5, {RGB565(0, 0, SHADE(8, 5)), 0}},
    {GFX_RES_SHIPB_6, {RGB565(0, 0, SHADE(8, 6)), 0}}, {GFX_RES_SHIPB_7, {RGB565(0, 0, SHADE(8, 7)), 0}},

    {GFX_RES_SHIPC_0, {RGB565(0, SHADE(16, 0) * 2, 0), 0}}, {GFX_RES_SHIPC_1, {RGB565(0, SHADE(16, 1) * 2, 0), 0}},
    {GFX_RES_SHIPC_2, {RGB565(0, SHADE(16, 2) * 2, 0), 0}}, {GFX_RES_SHIPC_3, {RGB565(0, SHADE(16, 3) * 2, 0), 0}},
    {GFX_RES_SHIPC_4, {RGB565(0, SHADE(16, 4) * 2, 0), 0}}, {GFX_RES_SHIPC_5, {RGB565(0, SHADE(16, 5) * 2, 0), 0}},
    {GFX_RES_SHIPC_6, {RGB565(0, SHADE(16, 6) * 2, 0), 0}}, {GFX_RES_SHIPC_7, {RGB565(0, SHADE(16, 7) * 2, 0), 0}},

    {GFX_RES_SHIPD_0, {RGB565(SHADE(8, 0), SHADE(16, 0) * 2, 0), 0}}, {GFX_RES_SHIPD_1, {RGB565(SHADE(8, 1), SHADE(16, 1) * 2, 0), 0}},
    {GFX_RES_SHIPD_2, {RGB565(SHADE(8, 2), SHADE(16, 2) * 2, 0), 0}}, {GFX_RES_SHIPD_3, {RGB565(SHADE(8, 3), SHADE(16, 3) * 2, 0), 0}},
    {GFX_RES_SHIPD_4, {RGB565(SHADE(8, 4), SHADE(16, 4) * 2, 0), 0}}, {GFX_RES_SHIPD_5, {RGB565(SHADE(8, 5), SHADE(16, 5) * 2, 0), 0}},
    {GFX_RES_SHIPD_6, {RGB565(SHADE(8, 6), SHADE(16, 6) * 2, 0), 0}}, {GFX_RES_SHIPD_7, {RGB565(SHADE(8, 7), SHADE(16, 7) * 2, 0), 0}},

    {GFX_RES_BASE, {RGB565(31, 63, 31), 0}},

    {GFX_RES_TORP_0, {RGB565(31, SHADE(31, 0), 0), 1}}, {GFX_RES_TORP_1, {RGB565(31, SHADE(31, 1), 0), 1}},
    {GFX_RES_TORP_2, {RGB565(31, SHADE(31, 2), 0), 1}}, {GFX_RES_TORP_3, {RGB565(31, SHADE(31, 3), 0), 1}},
    {GFX_RES_TORP_4, {RGB565(31, SHADE(31, 4), 0), 1}}, {GFX_RES_TORP_5, {RGB565(31, SHADE(31, 5), 0), 1}},

    {GFX_RES_EXPLOSION_0, {RGB565(31, SHADE(0, 0), 0), 1}}, {GFX_RES_EXPLOSION_1, {RGB565(31, SHADE(0, 1), 0), 1}},
    {GFX_RES_EXPLOSION_2, {RGB565(31, SHADE(0, 2), 0), 1}}, {GFX_RES_EXPLOSION_3, {RGB565(31, SHADE(0, 3), 0), 1}},
    {GFX_RES_EXPLOSION_4, {RGB565(31, SHADE(0, 4), 0), 1}}, {GFX_RES_EXPLOSION_5, {RGB565(31, SHADE(0, 5), 0), 1}},
    {GFX_RES_EXPLOSION_6, {RGB565(31, SHADE(0, 6), 0), 1}}, {GFX_RES_EXPLOSION_7, {RGB565(31, SHADE(0, 7), 0), 1}},
    {GFX_RES_EXPLOSION_8, {RGB565(31, SHADE(0, 8), 0), 1}}, {GFX_RES_EXPLOSION_9, {RGB565(31, SHADE(0, 9), 0), 1}},
    {GFX_RES_EXPLOSION_10, {RGB565(31, SHADE(0, 10), 0), 1}}, {GFX_RES_EXPLOSION_11, {RGB565(31, SHADE(0, 11), 0), 1}},
    {GFX_RES_EXPLOSION_12, {RGB565(31, 31, 0), 1}}, {GFX_RES_EXPLOSION_13, {RGB565(31, 31, 8), 1}},
    {GFX_RES_EXPLOSION_14, {RGB565(31, 31, 16), 1}}, {GFX_RES_EXPLOSION_15, {RGB565(31, 31, 31), 1}},

    {GFX_RES_HEALTH, {RGB565(0, 63, 0), 1}},
    {GFX_RES_COMET, {RGB565(20, 20, 20), 1}},
    {GFX_RES_UFO, {RGB565(0, 63, 31), 1}},

    {GFX_RES_TILEFEATURED, {RGB565(31, 63, 0), 0}},
    {GFX_RES_TILEMOVE, {RGB565(0, 63, 31), 0}},
    {GFX_RES_TILEATTACK, {RGB565(31, 0, 0), 0}},
    {GFX_RES_TILEWARN, {RGB565(31, 31, 0), 0}},

    {GFX_RES_STARANIM_0, {RGB565(SHADE(16, 0) * 2 - 1, SHADE(16, 0) * 2 - 1, SHADE(16, 0)), 1}},
    {GFX_RES_STARANIM_1, {RGB565(SHADE(16, 1) * 2 - 1, SHADE(16, 1) * 2 - 1, SHADE(16, 1)), 1}},
    {GFX_RES_STARANIM_2, {RGB565(SHADE(16, 2) * 2 - 1, SHADE(16, 2) * 2 - 1, SHADE(16, 2)), 1}},
    {GFX_RES_STARANIM_3, {RGB565(SHADE(16, 3) * 2 - 1, SHADE(16, 3) * 2 - 1, SHADE(16, 3)), 1}},

    {GFX_RES_NEBULA, {RGB565(20, 0, 31), 0}},

    {GFX_RES_FACTIONINDICATOR_0, {RGB565(31, 0, 0), 1}},
    {GFX_RES_FACTIONINDICATOR_1, {RGB565(0, 0, 31), 1}},
    {GFX_RES_FACTIONINDICATOR_2, {RGB565(0, 63, 0), 1}},
    {GFX_RES_FACTIONINDICATOR_3, {RGB565(31, 63, 0), 1}},

    {GFX_RES_HUMAN, {RGB565(31, 63, 31), 1}},
    {GFX_RES_CPUPLAYER, {RGB565(31, 0, 0), 1}},
};

static WebBitmap bitmapForId(unsigned short id) {
    unsigned int i;
    for (i = 0; i < sizeof(web_bitmapTable) / sizeof(web_bitmapTable[0]); i++) {
        if (web_bitmapTable[i].id == id) {
            return web_bitmapTable[i].bitmap;
        }
    }
    /* Unknown id: same "missing resource" fallback the Palm backend would
       hit if DmGetResource() found nothing, kept visible instead of NULL
       so an unmapped id is obvious on screen. */
    return (WebBitmap){RGB565(15, 15, 15), 0};
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
