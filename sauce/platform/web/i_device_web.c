#include "../i_device.h"

#include <emscripten.h>

#include "../../constants.h"
#include "web_framebuffer.h"

static int web_screenWidth = WEB_SCREEN_WIDTH;
static int web_screenHeight = WEB_SCREEN_HEIGHT;

/* Exported so the JS shell can size the canvas to the device before
   game_setup() runs (see web/index.html). */
EMSCRIPTEN_KEEPALIVE
void web_setScreenSize(int width, int height) {
    if (width > 0) {
        web_screenWidth = width;
    }
    if (height > 0) {
        web_screenHeight = height;
    }
}

IBool idevice_colorSupported() {
    return 1;
}

long idevice_currentDepth() {
    return 16;
}

unsigned long idevice_maxDepth() {
    return 16;
}

IBool idevice_isRunningMinimalOSVersion(unsigned char minVersion) {
    (void)minVersion;
    return 1;
}

IBool idevice_supportsHiDensity() {
    return 1;
}

Coordinate idevice_screenSize(void) {
    return (Coordinate){web_screenWidth, web_screenHeight};
}

IBool idevice_diaSupported() {
    return 0;
}

unsigned long idevice_setScreenDepth(unsigned long depth) {
    (void)depth;
    return 16;
}

int idevice_gridTileSize(Coordinate screenSize) {
    /* This is the platform the zoom feature exists for: pick the tile size
       so the full HEXGRID_COLS x HEXGRID_ROWS grid covers the screen on a
       phone-shaped (non-square) viewport, instead of relying on viewport
       panning at the historical fixed Palm tile size. The shared hexgrid
       code clamps this to HEXTILE_MINSIZE..HEXTILE_MAXSIZE.

       hexgrid_size() (game.c) returns, for a tile size T:
         width  = (HEXGRID_COLS + 0.5) * T + 5
         height = ((HEXGRID_ROWS - 1) * (1 - segFrac) + 1) * T + 5
       where segFrac = HEXTILE_SEGMENT_SIZE / HEXTILE_SIZE (hex rows overlap
       vertically). Solving each for T against the available screen width and
       height and taking the larger requirement ensures the grid fills both
       axes instead of leaving a blank band on whichever axis was ignored. */
    int availableHeight = screenSize.y - BOTTOMMENU_HEIGHT;
    double segFrac = (double)HEXTILE_SEGMENT_SIZE / HEXTILE_SIZE;
    double factorX = HEXGRID_COLS + 0.5;
    double factorY = (HEXGRID_ROWS - 1) * (1.0 - segFrac) + 1.0;
    double tileForWidth = (screenSize.x - 5) / factorX;
    double tileForHeight = (availableHeight - 5) / factorY;
    double tileSize = (tileForWidth > tileForHeight) ? tileForWidth : tileForHeight;
    return (int)tileSize;
}
