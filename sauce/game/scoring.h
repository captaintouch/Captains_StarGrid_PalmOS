#ifndef SCORING_H_
#define SCORING_H_
#define SCORING_SECTION  __attribute__ ((section ("scoring")))
#include <PalmOS.h>
#include "../constants.h"

typedef struct Score {
    int shipsDestroyed;
    int shipsLost;
    int shipsCaptured;
    int flagsCaptured;
    int flagsStolen;
} Score;

typedef struct LevelScore {
    int shipsDestroyed[GAMEMECHANICS_MAXPLAYERCOUNT];
    Boolean basesDestroyed[GAMEMECHANICS_MAXPLAYERCOUNT];
    int shipsCaptured[GAMEMECHANICS_MAXPLAYERCOUNT];
    Boolean flagsCaptured[GAMEMECHANICS_MAXPLAYERCOUNT];
    int flagsStolen;
} LevelScore;

DmResID scoring_rankForScore(Score score) SCORING_SECTION;
Score scoring_scoreFromLevelScores(LevelScore *levelScores, int faction) SCORING_SECTION;
int scoring_scoreValue(Score score) SCORING_SECTION;
int scoring_levelScoreValue(LevelScore *levelScores, int faction) SCORING_SECTION;

int scoring_totalDestroyedShips(LevelScore score) SCORING_SECTION;
int scoring_totalCapturedShips(LevelScore score) SCORING_SECTION;
int scoring_totalDestroyedBases(LevelScore score) SCORING_SECTION;

Score scoring_loadSavedScore() SCORING_SECTION; 

void scoring_reset() SCORING_SECTION; 
void scoring_saveScore(LevelScore *levelScores, int faction) SCORING_SECTION;

#endif