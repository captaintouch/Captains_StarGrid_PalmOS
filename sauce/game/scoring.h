#ifndef SCORING_H_
#define SCORING_H_
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

Score scoring_scoreFromLevelScores(LevelScore *levelScores, int faction);
Score scoring_appendScore(Score lScore, Score rScore);
int scoring_scoreValue(Score score);
int scoring_levelScoreValue(LevelScore *levelScores, int faction);

int scoring_totalDestroyedShips(LevelScore score);
int scoring_totalCapturedShips(LevelScore score);
int scoring_totalDestroyedBases(LevelScore score);

// functions: 
// - combine score (player saved total score + level score)
// - rank for score

#endif