#include "../i_device.h"

#include <stdlib.h>

#include "cli_framebuffer.h"

IBool idevice_colorSupported() {
    return 1;
}

long idevice_currentDepth() {
    return 8;
}

unsigned long idevice_maxDepth() {
    return 8;
}

IBool idevice_isRunningMinimalOSVersion(unsigned char minVersion) {
    (void)minVersion;
    return 1;
}

IBool idevice_supportsHiDensity() {
    return 0;
}

Coordinate idevice_screenSize(void) {
    return (Coordinate){CLI_SCREEN_WIDTH, CLI_SCREEN_HEIGHT};
}

IBool idevice_diaSupported() {
    return 0;
}

unsigned long idevice_setScreenDepth(unsigned long depth) {
    /* Nothing to switch on the terminal; report a non-zero old depth so the
       caller's "success" check passes. */
    (void)depth;
    return 8;
}

int idevice_gridTileSize(Coordinate screenSize) {
    /* Default: keep the fixed tile size (identical to the historical behavior).
       Set STARGRID_TILESIZE to a pixel value to exercise grid zoom for testing. */
    char *override = getenv("STARGRID_TILESIZE");
    (void)screenSize;
    if (override != NULL) {
        return atoi(override);
    }
    return 0;
}
