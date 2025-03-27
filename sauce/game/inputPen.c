#include "inputPen.h"

#include <PalmOS.h>

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penMoveEvent) {
        return;
    }
    pen->moving = eventPtr->eType == penMoveEvent;
    pen->touchCoordinate.x = eventPtr->screenX;
    pen->touchCoordinate.y = eventPtr->screenY;
    pen->wasUpdatedFlag = true;
}