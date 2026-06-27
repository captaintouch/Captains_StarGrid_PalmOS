#include "../i_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cli_input.h"

unsigned long isys_getTicks() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

unsigned long isys_ticksPerSecond() {
    return 1000;
}

int isys_random(int min, int max) {
    if (max <= min) {
        return min;
    }
    return min + (rand() % (max - min + 1));
}

void isys_sleepMs(unsigned long milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void isys_fatalError(char *message) {
    fprintf(stderr, "FATAL: %s\n", message);
    exit(1);
}

void isys_postOpenMenuEvent() {
    cli_input_postMenu();
}
