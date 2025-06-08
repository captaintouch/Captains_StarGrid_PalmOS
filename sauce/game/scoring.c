#include "scoring.h"

Score scoring_scoreFromLevelScore(LevelScore levelScore) {
    int i;
    Score score;
    MemSet(&score, sizeof(Score), 0);
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        if (levelScore.flagsCaptured[i]) {
            score.flagsCaptured++;
        }
        score.shipsDestroyed += levelScore.shipsDestroyed[i];
        score.shipsCaptured += levelScore.shipsCaptured[i];
        score.flagsCaptured += levelScore.flagsCaptured[i];
    }
    score.flagsStolen = levelScore.flagsStolen;
    score.shipsLost = levelScore.shipsLost;
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

int scoring_totalCapturedShips(LevelScore score) {
    int i, quantity = 0;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        quantity += score.shipsCaptured[i];
    }
    return quantity;
}