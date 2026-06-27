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
    /* This is the platform the zoom feature exists for: fit the full
       HEXGRID_COLS columns across the narrower screen dimension so the grid
       reads comfortably on a phone, instead of relying on viewport panning
       at the historical fixed Palm tile size. The shared hexgrid code clamps
       this to HEXTILE_MINSIZE..HEXTILE_MAXSIZE. */
    int narrowest = (screenSize.x < screenSize.y) ? screenSize.x : screenSize.y;
    return narrowest / HEXGRID_COLS;
}
