#ifndef CPU_LOGIC_H
#define CPU_LOGIC_H

#define CPULOGIC_SECTION  __attribute__ ((section ("cpulogic")))

#include "models.h"

typedef enum CPUAction {
    CPUACTION_NONE,
    CPUACTION_MOVE,
    CPUACTION_TORPEDOATTACK,
    CPUACTION_PHASERATTACK,
    CPUACTION_WARP,
    CPUACTION_BASE_SHOCKWAVE,
    CPUACTION_BASE_BUILDSHIP,
    CPUACTION_BASE_GRIDITEMHEALTH,
    CPUACTION_BASE_GRIDITEMTORPEDOES
} CPUAction;

typedef struct CPUStrategyResult {
    int score;
    CPUAction CPUAction;
    Pawn *target;
    Coordinate targetPosition;
    Boolean allowMoveToBase;
} CPUStrategyResult;

CPUStrategyResult cpuLogic_getStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount, GridItem *gridItems, int gridItemCount, int currentTurn, CPUFactionProfile factionProfile, Boolean cpuPlayersOnly) CPULOGIC_SECTION;

#endif
