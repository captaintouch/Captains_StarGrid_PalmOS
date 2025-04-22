#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (pen->blockUntilTicks > 0 && TimGetTicks() < pen->blockUntilTicks) {
        return;
    }
    pen->blockUntilTicks = 0;
    if (eventPtr->eType == penUpEvent) {
        pen->penUpOccured = true;
        return;
    }
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent) {
        return;
    }
    pen->moving = eventPtr->eType == penMoveEvent && !isEqualCoordinate(pen->touchCoordinate, (Coordinate){eventPtr->screenX, eventPtr->screenY});
    pen->touchCoordinate.x = eventPtr->screenX;
    pen->touchCoordinate.y = eventPtr->screenY;
    pen->wasUpdatedFlag = true;

    
}

void inputPen_temporarylyBlockPenInput(InputPen *pen) {
    pen->blockUntilTicks = TimGetTicks() + (float)SysTicksPerSecond() / 3.0;
}