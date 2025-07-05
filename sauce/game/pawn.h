#ifndef PAWN_H_
#define PAWN_H_
#include <PalmOS.h>
#include "models.h"

AppColor pawn_factionColor(UInt8 faction, Boolean colorSupport);
AppColor pawn_baseActivityIndicatorColor(Pawn *pawn, Boolean colorSupport, int currentTurn);

int pawn_baseTurnsLeft(UInt8 currentTurn, UInt8 lastActionTurn, BaseAction lastActionType);
#endif