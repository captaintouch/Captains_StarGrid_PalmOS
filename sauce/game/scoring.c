#include "scoring.h"
#include "../constants.h"
#include "../database.h"
#include <PalmOS.h>

SCORING_SECTION
const int rank_thresholds[RANK_COUNT] = {
    200, 280, 400, 550, 750, 1000, 1400, 2000, 2800, 4000,
    5500, 7500, 10000, 14000, 20000, 26000, 31000, 35000, 40000
};

SCORING_SECTION
Score scoring_scoreFromLevelScores(LevelScore *levelScores, int faction) {
    int i;
    Score score;
    MemSet(&score, sizeof(Score), 0);
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        if (levelScores[faction].flagsCaptured[i]) {
            score.flagsCaptured++;
        }
        score.shipsDestroyed += levelScores[faction].shipsDestroyed[i];
        score.shipsCaptured += levelScores[faction].shipsCaptured[i];
        score.flagsCaptured += levelScores[faction].flagsCaptured[i];
        score.shipsLost += levelScores[i].shipsDestroyed[faction];
    }
    score.flagsStolen = levelScores[faction].flagsStolen;
    return score;
}

SCORING_SECTION
static Score scoring_appendScore(Score lScore, Score rScore) {
    Score newScore;
    newScore.flagsCaptured = lScore.flagsCaptured + rScore.flagsCaptured;
    newScore.flagsStolen = lScore.flagsStolen + rScore.flagsStolen;
    newScore.shipsCaptured = lScore.shipsCaptured + rScore.shipsCaptured;
    newScore.shipsDestroyed = lScore.shipsDestroyed + rScore.shipsDestroyed;
    newScore.shipsLost = lScore.shipsLost + rScore.shipsLost;
    return newScore;
}

SCORING_SECTION
int scoring_totalDestroyedShips(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        quantity += score.shipsDestroyed[i];
    }
    return quantity;
}

SCORING_SECTION
int scoring_totalDestroyedBases(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        if (score.basesDestroyed[i]) {
            quantity++;
        }
    }
    return quantity;
}

SCORING_SECTION
int scoring_totalCapturedShips(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        quantity += score.shipsCaptured[i];
    }
    return quantity;
}

SCORING_SECTION
int scoring_scoreValue(Score score) {
    return -score.shipsLost + score.shipsDestroyed + score.shipsCaptured + score.flagsStolen * 2 + score.flagsCaptured * 3;
}

SCORING_SECTION
int scoring_levelScoreValue(LevelScore *levelScores, int faction) {
    Score score = scoring_scoreFromLevelScores(levelScores, faction);
    return scoring_scoreValue(score);
}

SCORING_SECTION
DmResID scoring_rankForScore(Score score) {
    int i;
    int value = scoring_scoreValue(score);
    if (value <= rank_thresholds[0]) return STRING_RANK0;
    if (value >= rank_thresholds[RANK_COUNT - 1]) return STRING_RANK0 + RANK_COUNT - 1;

    for (i = 0; i < RANK_COUNT - 1; ++i) {
        if (value < rank_thresholds[i + 1])
            return STRING_RANK0 + i;
    }

    return STRING_RANK0 + RANK_COUNT - 1;
}

SCORING_SECTION
Score scoring_loadSavedScore() {
    Score score = database_readScore();
    /*MemSet(&score, sizeof(Score), 0);
    score.flagsCaptured = 2;
    score.flagsStolen = 6;
    score.shipsCaptured = 3;
    score.shipsDestroyed = 10;
    score.shipsLost = 5;*/
    return score;
}

SCORING_SECTION
void scoring_saveScore(LevelScore *levelScores, int faction) {
    Score oldScore = database_readScore();
    Score score = scoring_scoreFromLevelScores(levelScores, faction);
    Score combinedScore = scoring_appendScore(oldScore, score);
    database_writeScore(&combinedScore);
}

SCORING_SECTION
void scoring_reset() {
    database_reset();
}