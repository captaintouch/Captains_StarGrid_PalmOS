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

int ceil(float a) {
    int intPart = (int)a;
    if (a > intPart) {
        return intPart + 1;
    } else {
        return intPart;
    }
}