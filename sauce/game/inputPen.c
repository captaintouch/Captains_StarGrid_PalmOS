#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    Boolean penUpTriggered = (pen->blockUpdatesUntilPenUp && eventPtr->eType == penUpEvent);
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent && !penUpTriggered) {
        return;
    }

    if (pen->blockUpdatesUntilPenUp) {
        pen->blockUpdatesUntilPenUp = eventPtr->eType != penUpEvent;
        pen->moving = false;
    } else {
        pen->moving = eventPtr->eType == penMoveEvent;
        pen->touchCoordinate.x = eventPtr->screenX;
        pen->touchCoordinate.y = eventPtr->screenY;
        pen->wasUpdatedFlag = true;
    }
}