#include "pawnActionMenuViewModel.h"
#include "pawn.h"
#include "../constants.h"

MenuActionType shipActions[] = {MenuActionTypeCancel, MenuActionTypeWarp, MenuActionTypeTorpedo, MenuActionTypePhaser, MenuActionTypeMove};
MenuActionType baseActions[] = {MenuActionTypeCancel, MenuActionTypeShockwave, MenuActionTypeBuildShip, MenuActionTypeHealthPack, MenuActionTypeTorpedoPack};

static UInt16 pawnActionMenuViewModel_textForActionType(MenuActionType actionType) {
    switch (actionType) {
        case MenuActionTypeMove:
            return STRING_MOVE;
        case MenuActionTypePhaser:
            return STRING_PHASER;
        case MenuActionTypeTorpedo:
            return STRING_TORPEDO;
        case MenuActionTypeWarp:
            return STRING_WARP;
        case MenuActionTypeShockwave:
            return STRING_SHOCKWAVE;
        case MenuActionTypeBuildShip:
            return STRING_BUILDSHIP;
        case MenuActionTypeHealthPack:
            return STRING_HEALTHPACK;
        case MenuActionTypeTorpedoPack:
            return STRING_TORPEDOPACK;
        case MenuActionTypeCancel:
            return STRING_CANCEL;
    }
}


static Boolean pawnActionMenuViewModel_isDisabled(MenuActionType actionType, Pawn *pawn, UInt8 currentTurn) {
    switch (actionType) {
        case MenuActionTypeMove:
        case MenuActionTypePhaser:
        case MenuActionTypeCancel:
            return false;
        case MenuActionTypeTorpedo:
            return pawn->inventory.torpedoCount == 0;
        case MenuActionTypeWarp:
            return pawn->warped || pawn->inventory.carryingFlag;
        case MenuActionTypeHealthPack:
        case MenuActionTypeTorpedoPack:
        case MenuActionTypeShockwave:
        case MenuActionTypeBuildShip:
            return pawn_baseTurnsLeft(currentTurn, pawn->inventory.baseActionLastActionTurn, pawn->inventory.lastBaseAction) > 0;
    }
}

static void appendTurnsRequiredForAction(MenuActionType actionType, char *text, UInt8 currentTurn, Pawn *pawn) {
    int turnsRequired;
    char valueText[20];
    if (pawn->type == PAWNTYPE_SHIP || !pawnActionMenuViewModel_isDisabled(actionType, pawn, currentTurn)) {
        return;
    }

    turnsRequired = pawn_baseTurnsLeft(currentTurn, pawn->inventory.baseActionLastActionTurn, pawn->inventory.lastBaseAction);
    if (turnsRequired > 0) {
        StrCat(text, " (in ");
        StrIToA(valueText, turnsRequired);
        StrCat(text, valueText);
        StrCat(text, " turns)");
    }
}

void pawnActionMenuViewModel_setupMenuForPawn(Pawn *pawn, Button **displayButtons, UInt8 *displayButtonCount, UInt8 currentTurn) {
    int i;
    MenuActionType *actions;
    int actionCount;
    Button *buttons;
    switch (pawn->type) {
        case PAWNTYPE_SHIP:
            actions = shipActions;
            actionCount = sizeof(shipActions) / sizeof(shipActions[0]);
            break;
        case PAWNTYPE_BASE:
            actions = baseActions;
            actionCount = sizeof(baseActions) / sizeof(baseActions[0]);
            break;
    }
    buttons = (Button *)MemPtrNew(sizeof(Button) * actionCount);

    for (i = 0; i < actionCount; i++) {
        MemHandle resourceHandle = DmGetResource(strRsc, pawnActionMenuViewModel_textForActionType(actions[i]));
        char *text = (char *)MemHandleLock(resourceHandle);
        buttons[i].text = (char *)MemPtrNew(StrLen(text) + 15);
        MemHandleUnlock(resourceHandle);
        DmReleaseResource(resourceHandle);
        StrCopy(buttons[i].text, text);
        appendTurnsRequiredForAction(actions[i], buttons[i].text, currentTurn, pawn);
        MemPtrResize(buttons[i].text, StrLen(buttons[i].text) + 1);
        buttons[i].length = StrLen(buttons[i].text);
        buttons[i].disabled = pawnActionMenuViewModel_isDisabled(actions[i], pawn, currentTurn);
    }
    *displayButtons = buttons;
    *displayButtonCount = actionCount;
}

MenuActionType pawnActionMenuViewModel_actionAtIndex(UInt8 index, Pawn *pawn) {
    return pawn->type == PAWNTYPE_SHIP ? shipActions[index] : baseActions[index];
}
