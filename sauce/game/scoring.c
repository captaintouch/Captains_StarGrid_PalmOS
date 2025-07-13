#include "scoring.h"

#include <PalmOS.h>

#include "../constants.h"
#include "../storage.h"

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
    const UInt32 rank_thresholds[RANK_COUNT] = {20, 28, 40, 55, 75, 100, 140, 200, 280, 400, 550, 750, 1000, 1400, 2000, 2600, 3100, 3500, 4000};
    int i;
    UInt32 scoreVal = scoring_scoreValue(score);
    DmResID rank = STRING_RANK0;
    for (i = 0; i < RANK_COUNT - 1; i++) {
        if (scoreVal >= rank_thresholds[i]) {
            rank = STRING_RANK0 + i + 1;
        }
    }
    return rank;
}

SCORING_SECTION
int scoring_rankValue(Score score) {
    return scoring_rankForScore(score) - STRING_RANK0;
}

SCORING_SECTION
int scoring_scoreNeededUntilNextRank(Score score) {
    const UInt32 rank_thresholds[RANK_COUNT] = {20, 28, 40, 55, 75, 100, 140, 200, 280, 400, 550, 750, 1000, 1400, 2000, 2600, 3100, 3500, 4000};
    int i;
    int rankScore = rank_thresholds[0];
    UInt32 scoreVal = scoring_scoreValue(score);
    for (i = 0; i < RANK_COUNT - 1; i++) {
        if (scoreVal >= rank_thresholds[i] && i + 1 < RANK_COUNT) {
            rankScore = rank_thresholds[i + 1];
        }
    }
    return rankScore - scoreVal;
}

SCORING_SECTION
Score scoring_loadSavedScore() {
    return storage_readScore();
}

SCORING_SECTION
void scoring_saveScore(LevelScore *levelScores, int faction) {
    Score oldScore = storage_readScore();
    Score score = scoring_scoreFromLevelScores(levelScores, faction);
    Score combinedScore = scoring_appendScore(oldScore, score);
    storage_writeScore(&combinedScore);
}

SCORING_SECTION
void scoring_reset() {
    storage_reset();
}