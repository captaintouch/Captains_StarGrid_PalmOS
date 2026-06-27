#include "../i_system.h"

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>

#include "web_input.h"

unsigned long isys_getTicks() {
    return (unsigned long)emscripten_get_now(); /* milliseconds since page load */
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
    /* The browser's main loop can't block without freezing the page (no
       blocking sleep without Asyncify). The CPU "thinking" pauses that use
       this are cosmetic, so skipping them is the right tradeoff here. */
    (void)milliseconds;
}

void isys_fatalError(char *message) {
    fprintf(stderr, "FATAL: %s\n", message);
    EM_ASM({ alert(UTF8ToString($0)); }, message);
    exit(1);
}

void isys_postOpenMenuEvent() {
    web_input_postMenu();
}
