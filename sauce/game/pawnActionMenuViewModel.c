#include "pawnActionMenuViewModel.h"
#include "../constants.h"

const MenuActionType allActions[] = {MenuActionTypeCancel, MenuActionTypeWarp, MenuActionTypeTorpedo, MenuActionTypePhaser, MenuActionTypeMove};

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
        case MenuActionTypeCancel:
            return STRING_CANCEL;
    }
}

static Boolean pawnActionMenuViewModel_isDisabled(MenuActionType actionType, Pawn *pawn) {
    switch (actionType) {
        case MenuActionTypeMove:
        case MenuActionTypePhaser:
        case MenuActionTypeCancel:
            return false;
        case MenuActionTypeTorpedo:
            return pawn->inventory.torpedoCount == 0;
        case MenuActionTypeWarp:
            return pawn->warped || pawn->inventory.carryingFlag;
    }
}

void pawnActionMenuViewModel_setupMenuForPawn(Pawn *pawn, Button **displayButtons, UInt8 *displayButtonCount) {
    int i;
    MenuActionType *actions = allActions;
    int actionCount = sizeof(allActions) / sizeof(allActions[0]);
    Button *buttons = (Button *)MemPtrNew(sizeof(Button) * actionCount);

    for (i = 0; i < actionCount; i++) {
        MemHandle resourceHandle = DmGetResource(strRsc, pawnActionMenuViewModel_textForActionType(actions[i]));
        char *text = (char *)MemHandleLock(resourceHandle);
        buttons[i].text = (char *)MemPtrNew(StrLen(text) + 1);
        MemHandleUnlock(resourceHandle);
        DmReleaseResource(resourceHandle);
        StrCopy(buttons[i].text, text);
        buttons[i].length = StrLen(text);
        buttons[i].disabled = pawnActionMenuViewModel_isDisabled(actions[i], pawn);
    }
    *displayButtons = buttons;
    *displayButtonCount = actionCount;
}

MenuActionType pawnActionMenuViewModel_actionAtIndex(UInt8 index, Pawn *pawn) {
    return allActions[index];
}