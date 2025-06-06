#include "mathIsFun.h"
#include <PalmOS.h>

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


float remapToMax(float a, float maxValue) {
    float result = a;
    while (result >= maxValue) {
        result -= maxValue;
    }
    return result;
}

int random(int min, int max) {
    int seed = TimGetTicks();
    int value;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    value = min + (seed % (max - min + 1));
    return fmin(fmax(value, min), max);
}

void mathIsFun_shuffleIndices(int *indices, int totalCount) {
    int i;
    for (i = 0; i < totalCount; i++) {
        indices[i] = i;
    }

    for (i = 0; i < totalCount; i++) {
        int t;
        int j = random(0, totalCount - 1);
        if (j == i) {
            continue;
        }
        t = indices[j];
        indices[j] = indices[i];
        indices[i] = t;
    }
}