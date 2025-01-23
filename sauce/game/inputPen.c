#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent) {
        return;
    }

    pen->touchCoordinate.x = eventPtr->screenX;
    pen->touchCoordinate.y = eventPtr->screenY;

    pen->moving = eventPtr->eType == penMoveEvent;
    pen->wasUpdatedFlag = true;
}