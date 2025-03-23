#ifndef GAMEACTIONLOGIC_H_
#define GAMEACTIONLOGIC_H_
#include <PalmOS.h>
#include "models.h"
#include "gamesession.h"

void gameActionLogic_afterAttack();
Boolean gameActionLogic_afterMove();

void gameActionLogic_clearAttack();

void gameActionLogic_scheduleMovement(Pawn *targetPawn, Coordinate selectedTile);
void gameActionLogic_scheduleAttack(Pawn *targetPawn, Coordinate selectedTile, TargetSelectionType attackType);
UInt8 gameActionLogic_maxRange(TargetSelectionType targetSelectionType);

#endif