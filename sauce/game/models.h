#ifndef MODELS_H_
#define MODELS_H_

typedef struct Coordinate {
    int x;
    int y;
} Coordinate;

typedef struct Line {
    Coordinate startpoint;
    Coordinate endpoint;
} Line;

#endif