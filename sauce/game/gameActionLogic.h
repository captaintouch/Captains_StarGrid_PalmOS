#ifndef GAMEACTIONLOGIC_H_
#define GAMEACTIONLOGIC_H_
#include <PalmOS.h>
#include "models.h"

void gameActionLogic_afterAttack();
Boolean gameActionLogic_afterMove();

void gameActionLogic_scheduleMovement(Pawn *targetPawn, Coordinate selectedTile);

#endif