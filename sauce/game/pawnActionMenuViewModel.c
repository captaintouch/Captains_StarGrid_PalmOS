#include "pawnActionMenuViewModel.h"

// TODO: Move literals to resource file
#define moveText "Move"
#define phaserText "Phaser"
#define torpedoText "Torpedo"
#define cloakText "Cloak"
#define decloakText "Decloak"
#define cancelText "Cancel"

const MenuActionType allActions[] = {MenuActionTypeCancel, MenuActionTypeCloak, MenuActionTypeTorpedo, MenuActionTypePhaser, MenuActionTypeMove};
const MenuActionType cloakedActions[] = {MenuActionTypeCancel, MenuActionTypeCloak, MenuActionTypeMove};

static char *pawnActionMenuViewModel_textForActionType(MenuActionType actionType, Boolean cloaked) {
    switch (actionType) {
        case MenuActionTypeMove:
            return moveText;
        case MenuActionTypePhaser:
            return phaserText;
        case MenuActionTypeTorpedo:
            return torpedoText;
        case MenuActionTypeCloak:
            return cloaked ? decloakText : cloakText;
        case MenuActionTypeCancel:
            return cancelText;
    }
}

void pawnActionMenuViewModel_setupMenuForPawn(Pawn *pawn, Button **displayButtons, int *displayButtonCount) {
    int i;
    MenuActionType *actions = pawn->cloaked ? cloakedActions : allActions;
    int actionCount = pawn->cloaked ? sizeof(cloakedActions) / sizeof(cloakedActions[0]) : sizeof(allActions) / sizeof(allActions[0]);
    Button *buttons = (Button *)MemPtrNew(sizeof(Button) * actionCount);

    for (i = 0; i < actionCount; i++) {
        Char *text = pawnActionMenuViewModel_textForActionType(actions[i], pawn->cloaked);
        buttons[i].text = (char *)MemPtrNew(StrLen(text) + 1);
        StrCopy(buttons[i].text, text);
        buttons[i].length = StrLen(text);
    }
    *displayButtons = buttons;
    *displayButtonCount = actionCount;
}

MenuActionType pawnActionMenuViewModel_actionAtIndex(int index, Pawn *pawn) {
    if (pawn->cloaked) {
        return cloakedActions[index];
    }
    return allActions[index];
}