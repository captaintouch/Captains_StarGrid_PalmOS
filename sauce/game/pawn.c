#include "colors.h"
#include "pawn.h"
#include "../constants.h"

AppColor pawn_factionColor(UInt8 faction, Boolean colorSupport) {
    if (!colorSupport) {
        return BELIZEHOLE;
    }
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

int pawn_baseTurnsLeft(UInt8 currentTurn, UInt8 lastActionTurn, BaseAction lastActionType) {
    int requiredTurns = GAMEMECHANICS_BASEACTIONREQUIREDTURNS;
    switch (lastActionType) {
        case BASEACTION_NONE:
        case BASEACTION_SHOCKWAVE:
        break;
        case BASEACTION_BUILD_SHIP:
            requiredTurns *= 1.5;
    }
    return lastActionTurn + requiredTurns - currentTurn;
}

AppColor pawn_baseActivityIndicatorColor(Pawn *pawn, Boolean colorSupport, int currentTurn) {
    if (pawn_baseTurnsLeft(currentTurn, pawn->inventory.baseActionLastActionTurn, pawn->inventory.lastBaseAction) > 0) { // Busy building
        switch (pawn->inventory.lastBaseAction) {
            case BASEACTION_NONE:
            case BASEACTION_SHOCKWAVE:
                return SUNFLOWER;
            case BASEACTION_BUILD_SHIP:
                return ALIZARIN; 
        }
    } else {
        return EMERALD;
    }
}