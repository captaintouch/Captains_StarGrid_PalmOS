#include "mathIsFun.h"

float fmin(float a, float b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

float fmax(float a, float b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
