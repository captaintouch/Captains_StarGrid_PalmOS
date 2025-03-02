#include "colors.h"
#include "pawn.h"

AppColor pawn_factionColor(UInt8 faction) {
    switch (faction % 4) {
        case 0:
            return EMERALD;
        case 1:
            return BELIZEHOLE;
        case 2:
            return SUNFLOWER;
        case 3:
            return ALIZARIN;
        default:
            return DRACULAORCHID;  // Default case, should not be reached
    }
}