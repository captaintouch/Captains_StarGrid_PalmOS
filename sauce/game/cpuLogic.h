#ifndef CPU_LOGIC_H
#define CPU_LOGIC_H
#include "models.h"

typedef enum CPUAction {
    CPUACTION_NONE,
    CPUACTION_MOVE,
    CPUACTION_TORPEDOATTACK,
    CPUACTION_PHASERATTACK,
    CPUACTION_CLOAK
} CPUAction;

typedef struct CPUStrategyResult {
    int score;
    CPUAction CPUAction;
    Pawn *target;
} CPUStrategyResult;

CPUStrategyResult cpuLogic_getStrategy(Pawn *pawn, Pawn *allPawns, int totalPawnCount);

#endif