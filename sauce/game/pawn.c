#include "colors.h"
#include "pawn.h"

AppColor pawn_factionColor(UInt8 faction) {
    switch (faction % 5) {
        case 0:
            return EMERALD;
        case 1:
            return CLOUDS;
        case 2:
            return SUNFLOWER;
        case 3:
            return BELIZEHOLE;
        default:
            return DRACULAORCHID;  // Default case, should not be reached
    }
}