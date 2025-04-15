#include "pawnActionMenuViewModel.h"

// TODO: Move literals to resource file
#define moveText "Move"
#define phaserText "Phaser"
#define torpedoText "Torpedo"
#define warpText "Warp home"
#define cancelText "Cancel"

const MenuActionType allActions[] = {MenuActionTypeCancel, MenuActionTypeWarp, MenuActionTypeTorpedo, MenuActionTypePhaser, MenuActionTypeMove};

static char *pawnActionMenuViewModel_textForActionType(MenuActionType actionType) {
    switch (actionType) {
        case MenuActionTypeMove:
            return moveText;
        case MenuActionTypePhaser:
            return phaserText;
        case MenuActionTypeTorpedo:
            return torpedoText;
        case MenuActionTypeWarp:
            return warpText;
        case MenuActionTypeCancel:
            return cancelText;
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
        Char *text = pawnActionMenuViewModel_textForActionType(actions[i]);
        buttons[i].text = (char *)MemPtrNew(StrLen(text) + 1);
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