#include "colors.h"

#include <PalmOS.h>

#include "Bitmap.h"
#include "Window.h"

IndexedColorType colors_reference[7];

static RGBColorType colors_rgb(Int16 red, Int16 green, Int16 blue) {
    RGBColorType color;
    color.r = red;
    color.g = green;
    color.b = blue;
    return color;
}

static IndexedColorType colors_rgbFor(AppColor colorType) {
    RGBColorType color;
    switch (colorType) {
        case ALIZARIN:
            color = colors_rgb(231, 76, 60);
            break;
        case BELIZEHOLE:
            color = colors_rgb(41, 128, 185);
            break;
        case EMERALD:
            color = colors_rgb(46, 204, 113);
            break;
        case CLOUDS:
            color = colors_rgb(255, 255, 255);
            break;
        case DRACULAORCHID:
            color = colors_rgb(45, 52, 54);
            break;
        case ASBESTOS:
            color = colors_rgb(127, 140, 141);
            break;
        case SUNFLOWER:
            color = colors_rgb(241, 196, 15);
            break;
    }
    return WinRGBToIndex(&color);
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
        colors_reference[BELIZEHOLE] = 6;
        colors_reference[DRACULAORCHID] = 0;
        colors_reference[EMERALD] = 9;
        colors_reference[CLOUDS] = 0;
        colors_reference[ASBESTOS] = 15;
        colors_reference[SUNFLOWER] = 7;

        if (depth == 8) {
            for (i = 0; i < 7; i++) {
                colors_reference[i] *= 2;
            }
        }
    }
}
