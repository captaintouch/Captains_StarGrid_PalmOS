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
        pen->penUpOccured = true;
        pen->touchCoordinate.x = -999;
        pen->touchCoordinate.y = -999;
        pen->wasUpdatedFlag = true;
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
    pen->blockUntilTicks = TimGetTicks() + SysTicksPerSecond() / 3;
}