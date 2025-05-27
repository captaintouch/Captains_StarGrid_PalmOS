#ifndef GAMEACTIONLOGIC_H_
#define GAMEACTIONLOGIC_H_
#include <PalmOS.h>
#include "models.h"
#include "gamesession.h"

void gameActionLogic_afterAttack();
Boolean gameActionLogic_afterMove();

void gameActionLogic_clearMovement();
void gameActionLogic_clearAttack();
void gameActionLogic_clearShockwave();

void gameActionLogic_scheduleWarp(Pawn *sourcePawn, Coordinate target);
void gameActionLogic_scheduleMovement(Pawn *sourcePawn, Pawn *targetPawn, Coordinate selectedTile);
void gameActionLogic_scheduleAttack(Pawn *targetPawn, Coordinate selectedTile, TargetSelectionType attackType);
void gameActionLogic_scheduleShockwave(Pawn *basePawn);
UInt8 gameActionLogic_maxRange(TargetSelectionType targetSelectionType);

#endif