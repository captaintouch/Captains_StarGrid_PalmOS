#include "colors.h"

#include "../platform/i_draw.h"

IndexedColorType colors_reference[7];

static IndexedColorType colors_rgbFor(AppColor colorType) {
    switch (colorType) {
        case ALIZARIN:
            return idraw_indexForRGB(231, 76, 60);
        case BELIZEHOLE:
            return idraw_indexForRGB(41, 128, 185);
        case EMERALD:
            return idraw_indexForRGB(46, 204, 113);
        case CLOUDS:
            return idraw_indexForRGB(255, 255, 255);
        case DRACULAORCHID:
            return idraw_indexForRGB(45, 52, 54);
        case ASBESTOS:
            return idraw_indexForRGB(127, 140, 141);
        case SUNFLOWER:
            return idraw_indexForRGB(241, 196, 15);
    }
    return 0;
}
void colors_setupReferenceColors(Boolean colorSupport, UInt32 depth) {
    if (colorSupport) {
        colors_reference[ALIZARIN] = colors_rgbFor(ALIZARIN);
        colors_reference[BELIZEHOLE] = colors_rgbFor(BELIZEHOLE);
        colors_reference[DRACULAORCHID] = colors_rgbFor(DRACULAORCHID);
        colors_reference[EMERALD] = colors_rgbFor(EMERALD);
        colors_reference[CLOUDS] = colors_rgbFor(CLOUDS);
        colors_reference[ASBESTOS] = colors_rgbFor(ASBESTOS);
        colors_reference[SUNFLOWER] = colors_rgbFor(SUNFLOWER);
    } else {
        int i;
        colors_reference[ALIZARIN] = 9;
        colors_reference[BELIZEHOLE] = 7;
        colors_reference[DRACULAORCHID] = 0;
        colors_reference[EMERALD] = 9;
        colors_reference[CLOUDS] = 0;
        colors_reference[ASBESTOS] = 15;
        colors_reference[SUNFLOWER] = 4;

        if (depth == 8) {
            for (i = 0; i < 7; i++) {
                colors_reference[i] *= 2;
            }
        }
    }
}
