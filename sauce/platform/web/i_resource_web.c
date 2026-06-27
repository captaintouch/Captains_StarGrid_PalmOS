#include "../i_resource.h"

#include <stdlib.h>

#include "../../constants.h"
#include "../../graphicResources.h"
#include "web_bitmap.h"

/* ---- Bitmaps: each "bitmap" is a stub handle carrying a representative
   color + shape (see web_bitmap.h / i_draw_web.c), in place of real art. ---- */

#define RGB565(r, g, b) (((unsigned short)((r) & 0x1F) << 11) | ((unsigned short)((g) & 0x3F) << 5) | (unsigned short)((b) & 0x1F))

static WebBitmap bitmapForId(unsigned short id) {
    if (id >= GFX_RES_SHIPA_0 && id <= GFX_RES_SHIPA_7) return (WebBitmap){RGB565(31, 0, 0), 0};
    if (id >= GFX_RES_SHIPB_0 && id <= GFX_RES_SHIPB_7) return (WebBitmap){RGB565(0, 0, 31), 0};
    if (id >= GFX_RES_SHIPC_0 && id <= GFX_RES_SHIPC_7) return (WebBitmap){RGB565(0, 63, 0), 0};
    if (id >= GFX_RES_SHIPD_0 && id <= GFX_RES_SHIPD_7) return (WebBitmap){RGB565(31, 63, 0), 0};
    if (id == GFX_RES_BASE) return (WebBitmap){RGB565(31, 63, 31), 0};
    if (id >= GFX_RES_TORP_0 && id <= GFX_RES_TORP_5) return (WebBitmap){RGB565(31, 31, 0), 1};
    if (id >= GFX_RES_EXPLOSION_0 && id <= GFX_RES_EXPLOSION_15) return (WebBitmap){RGB565(31, 16, 0), 1};
    if (id == GFX_RES_HEALTH) return (WebBitmap){RGB565(0, 63, 0), 1};
    if (id == GFX_RES_COMET) return (WebBitmap){RGB565(20, 20, 20), 1};
    if (id == GFX_RES_UFO) return (WebBitmap){RGB565(0, 63, 31), 1};
    if (id >= GFX_RES_TILEFEATURED && id <= GFX_RES_TILEWARN) return (WebBitmap){RGB565(15, 15, 15), 0};
    if (id >= GFX_RES_STARANIM_0 && id <= GFX_RES_STARANIM_3) return (WebBitmap){RGB565(31, 63, 31), 1};
    if (id == GFX_RES_NEBULA) return (WebBitmap){RGB565(20, 0, 31), 0};
    if (id >= GFX_RES_FACTIONINDICATOR_0 && id <= GFX_RES_FACTIONINDICATOR_3) return (WebBitmap){RGB565(31, 31, 31), 1};
    if (id == GFX_RES_HUMAN) return (WebBitmap){RGB565(31, 63, 31), 1};
    if (id == GFX_RES_CPUPLAYER) return (WebBitmap){RGB565(31, 0, 0), 1};
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
