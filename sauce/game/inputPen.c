#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (pen->disableUpdateUntilPenUp) {
        if (eventPtr->eType != penUpEvent) {
            return;
        }
        pen->disableUpdateUntilPenUp = false;
    }
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent && eventPtr->eType != penUpEvent) {
        return;
    }
    pen->moving = eventPtr->eType == penMoveEvent;
    pen->touchCoordinate.x = eventPtr->screenX;
    pen->touchCoordinate.y = eventPtr->screenY;
    pen->penUp = eventPtr->eType == penUpEvent;
    pen->wasUpdatedFlag = true;
}