#ifndef PAWNACTIONMENUVIEWMODEL_H_
#define PAWNACTIONMENUVIEWMODEL_H_
#include "models.h"
#include "bottomMenu.h"

typedef enum {
    MenuActionTypeMove,
    MenuActionTypePhaser,
    MenuActionTypeTorpedo,
    MenuActionTypeWarp,
    MenuActionTypeShockwave,
    MenuActionTypeBuildShip,
    MenuActionTypeCancel
} MenuActionType;

void pawnActionMenuViewModel_setupMenuForPawn(Pawn *pawn, Button **displayButtons, UInt8 *displayButtonCount, UInt8 currentTurn);
MenuActionType pawnActionMenuViewModel_actionAtIndex(UInt8 index, Pawn *pawn);
#endif