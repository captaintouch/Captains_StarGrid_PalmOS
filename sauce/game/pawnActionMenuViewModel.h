#ifndef PAWNACTIONMENUVIEWMODEL_H_
#define PAWNACTIONMENUVIEWMODEL_H_
#include "models.h"
#include "bottomMenu.h"

typedef enum {
    MenuActionTypeMove,
    MenuActionTypePhaser,
    MenuActionTypeTorpedo,
    MenuActionTypeCloak,
    MenuActionTypeCancel
} MenuActionType;

void pawnActionMenuViewModel_setupMenuForPawn(Pawn *pawn, Button **displayButtons, int *displayButtonCount);
MenuActionType pawnActionMenuViewModel_actionAtIndex(int index, Pawn *pawn);
#endif