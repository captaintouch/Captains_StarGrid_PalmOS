#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"
#ifdef DEBUG
#include "drawhelper.h"
#endif

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (pen->blockUntilTicks > 0 && TimGetTicks() < pen->blockUntilTicks) {
        return;
    }
    pen->blockUntilTicks = 0;
    if (eventPtr->eType == penUpEvent) {
        pen->moving = false;
        pen->wasUpdatedFlag = true;
        return;
    }
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent) {
        return;
    }
    pen->moving = eventPtr->eType == penMoveEvent;
    pen->touchCoordinate.x = eventPtr->screenX;
    pen->touchCoordinate.y = eventPtr->screenY;
    pen->wasUpdatedFlag = true;

    #ifdef DEBUG
    drawhelper_drawTextWithValue("X:", eventPtr->screenX, (Coordinate){eventPtr->screenX, eventPtr->screenY});
    drawhelper_drawTextWithValue("Y:", eventPtr->screenY, (Coordinate){eventPtr->screenX, eventPtr->screenY + 10});
    #endif
}

void inputPen_temporarylyBlockPenInput(InputPen *pen) {
    pen->blockUntilTicks = TimGetTicks() + SysTicksPerSecond() / 10;
}