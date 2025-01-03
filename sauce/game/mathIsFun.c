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

double sqrt(double number) {
    double guess;
    const double epsilon = 1e-3;
    if (number < 0) {
        return -1;
    }
    guess = number / 2.0;

    while (1) {
        double new_guess = 0.5 * (guess + number / guess);
        if (fabs(new_guess - guess) < epsilon) {
            return new_guess;
        }
        guess = new_guess;
    }
}