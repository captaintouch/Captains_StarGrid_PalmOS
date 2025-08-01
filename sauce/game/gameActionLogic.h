#ifndef GAMEACTIONLOGIC_H_
#define GAMEACTIONLOGIC_H_
#define GAMEACTIONLOGIC_SECTION  __attribute__ ((section ("gameact")))
#include <PalmOS.h>
#include "models.h"
#include "gamesession.h"

void gameActionLogic_afterAttack(GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_afterExplosion(GameSession *session) GAMEACTIONLOGIC_SECTION;  
Boolean gameActionLogic_afterMove(GameSession *session) GAMEACTIONLOGIC_SECTION;

void gameActionLogic_clearMovement(GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_clearAttack(GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_clearShockwave(GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_clearSceneAnimation(GameSession *session) GAMEACTIONLOGIC_SECTION; 

void gameActionLogic_scheduleWarp(Pawn *sourcePawn, Coordinate target, GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_scheduleMovement(Pawn *sourcePawn, Pawn *targetPawn, Coordinate selectedTile, GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_scheduleAttack(Pawn *targetPawn, Coordinate selectedTile, TargetSelectionType attackType, GameSession *session) GAMEACTIONLOGIC_SECTION;
void gameActionLogic_scheduleShockwave(Pawn *basePawn, GameSession *session) GAMEACTIONLOGIC_SECTION;
UInt8 gameActionLogic_maxRange(TargetSelectionType targetSelectionType) GAMEACTIONLOGIC_SECTION;
Boolean gameActionLogic_humanShipsLeft(GameSession *session) GAMEACTIONLOGIC_SECTION;

#endif
