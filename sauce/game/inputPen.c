#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr) {
    if (eventPtr->eType != penDownEvent) {
        return;
    }
    if (eventPtr->eType == penDownEvent) {
        pen->touchCoordinate.x = eventPtr->screenX;
        pen->touchCoordinate.y = eventPtr->screenY;
    }
    
    pen->touching = eventPtr->penDown;
    pen->wasUpdatedFlag = true;
}