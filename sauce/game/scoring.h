#ifndef SCORING_H_
#define SCORING_H_

typedef struct Score {
    int shipsDestroyed;
    int shipsLost;
    int shipsCaptured;
    int flagsCaptured;
    int flagsStolen;
} Score;

// functions: 
// - combine score (player saved total score + level score)
// - rank for score

#endif