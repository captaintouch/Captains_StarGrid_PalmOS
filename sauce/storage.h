#ifndef STORAGE_H_
#define STORAGE_H_
#define DB_SECTION  __attribute__ ((section ("storage")))
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
void storage_reset() DB_SECTION;
Score storage_readScore() DB_SECTION;
void storage_writeScore(Score *score) DB_SECTION;

// GAME STATE
void storage_saveGameState(int currentTurn, int pawnCount, int factionCount, int factionTurn, Pawn *pawns, Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT], LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT]) DB_SECTION;
Boolean storage_restoreGameState(UInt8 *currentTurn, int *pawnCount, int *factionCount, int *factionTurn, Pawn **pawns, Faction *factions, LevelScore *scores) DB_SECTION;
#endif