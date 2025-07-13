#ifndef STORAGE_H_
#define STORAGE_H_
#define STORAGE_SECTION  __attribute__ ((section ("storage")))
#include "game/scoring.h"
#include "game/models.h"

typedef struct GameRestorableSessionData {
    int pawnCount;
    Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT];
    LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT];
    int factionCount;
    int factionTurn;
    int currentTurn;
} GameRestorableSessionData;

// DATABASE
void storage_reset() STORAGE_SECTION;
Score storage_readScore() STORAGE_SECTION;
void storage_writeScore(Score *score) STORAGE_SECTION;

// GAME STATE
void storage_saveGameState(int currentTurn, int pawnCount, int factionCount, int factionTurn, Pawn *pawns, Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT], LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT]) STORAGE_SECTION;
Boolean storage_restoreGameState(UInt8 *currentTurn, int *pawnCount, int *factionCount, int *factionTurn, Pawn **pawns, Faction *factions, LevelScore *scores) STORAGE_SECTION;
#endif