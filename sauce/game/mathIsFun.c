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

int floor(float a) {
    int intPart = (int)a;
    if (a < intPart) {
        return intPart - 1;
    } else {
        return intPart;
    }
}

double fabs(double a) {
    if (a < 0) {
        return -a;
    } else {
        return a;
    }
}

int round(float a) {
    if (a >= 0) {
        return (int)(a + 0.5);
    } else {
        return (int)(a - 0.5);
    }
}