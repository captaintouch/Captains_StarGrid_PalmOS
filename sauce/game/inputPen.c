#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr, int xOffset, int yOffset) {
    if (eventPtr->eType != penDownEvent) {
        return;
    }
    if (eventPtr->eType == penDownEvent) {
        pen->touchCoordinate.x = eventPtr->screenX + xOffset;
        pen->touchCoordinate.y = eventPtr->screenY + yOffset;
    }
    
    pen->touching = eventPtr->penDown;
    pen->wasUpdatedFlag = true;
}