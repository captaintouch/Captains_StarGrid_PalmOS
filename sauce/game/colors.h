#ifndef COLORS_H_
#define COLORS_H_

#include <PalmOS.h>

typedef enum {
    ALIZARIN,
    BELIZEHOLE,
    DRACULAORCHID,
    EMERALD,
    CLOUDS,
    ASBESTOS,
    SUNFLOWER,
} AppColor;

IndexedColorType colors_reference[7];

void colors_setupReferenceColors(Boolean colorSupport, UInt32 depth);

#endif
