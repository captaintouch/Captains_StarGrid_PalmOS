#include "i_system.h"

#include <PalmOS.h>

unsigned long isys_getTicks() {
    return TimGetTicks();
}

unsigned long isys_ticksPerSecond() {
    return SysTicksPerSecond();
}

int isys_random(int min, int max) {
    return SysRandom(0) % (max - min + 1) + min;
}

void isys_sleepMs(unsigned long milliseconds) {
    UInt32 startTicks = TimGetTicks();
    UInt32 delayTicks = (milliseconds * SysTicksPerSecond()) / 1000;
    EventType event;

    while ((TimGetTicks() - startTicks) < delayTicks) {
        EvtGetEvent(&event, delayTicks);
        if (event.eType != nilEvent) {
            EvtAddEventToQueue(&event);  // Put it back if needed
        }
    }
}

void isys_fatalError(char *message) {
    ErrFatalDisplay(message);
}

void isys_postOpenMenuEvent() {
    EventType event;
    MemSet(&event, sizeof(EventType), 0);
    event.eType = keyDownEvent;
    event.data.keyDown.chr = vchrMenu;
    event.data.keyDown.modifiers = commandKeyMask;
    EvtAddEventToQueue(&event);
}
