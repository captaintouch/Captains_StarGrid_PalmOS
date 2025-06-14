#include "scoring.h"

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

Score scoring_appendScore(Score lScore, Score rScore) {
    Score newScore;
    newScore.flagsCaptured = lScore.flagsCaptured + rScore.flagsCaptured;
    newScore.flagsStolen = lScore.flagsStolen + rScore.flagsStolen;
    newScore.shipsCaptured = lScore.shipsCaptured + rScore.shipsCaptured;
    newScore.shipsDestroyed = lScore.shipsDestroyed + rScore.shipsDestroyed;
    newScore.shipsLost = lScore.shipsLost + rScore.shipsLost;
    return newScore;
}

int scoring_totalDestroyedShips(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        quantity += score.shipsDestroyed[i];
    }
    return quantity;
}

int scoring_totalDestroyedBases(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        if (score.basesDestroyed[i]) {
            quantity++;
        }
    }
    return quantity;
}

int scoring_totalCapturedShips(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        quantity += score.shipsCaptured[i];
    }
    return quantity;
}

int scoring_scoreValue(Score score) {
    return -score.shipsLost + score.shipsDestroyed + score.shipsCaptured + score.flagsStolen * 2 + score.flagsCaptured * 3;
}

int scoring_levelScoreValue(LevelScore *levelScores, int faction) {
    Score score = scoring_scoreFromLevelScores(levelScores, faction);
    return scoring_scoreValue(score);
}