#include "inputPen.h"

#include "../platform/i_system.h"

#include "models.h"

void inputPen_updateEventDetails(InputPen *pen, InputEvent *event) {
    if (pen->blockUntilTicks > 0 && isys_getTicks() < pen->blockUntilTicks) {
        return;
    }
    pen->blockUntilTicks = 0;
    if (event->type == IEVENT_PENUP) {
        pen->penUpOccured = true;
        return;
    }
    if (event->type != IEVENT_PENDOWN && event->type != IEVENT_PENMOVE) {
        return;
    }
    pen->moving = event->type == IEVENT_PENMOVE && !isEqualCoordinate(pen->touchCoordinate, event->point);
    pen->touchCoordinate = event->point;
    pen->wasUpdatedFlag = true;
}

void inputPen_temporarylyBlockPenInput(InputPen *pen) {
    pen->blockUntilTicks = isys_getTicks() + (float)isys_ticksPerSecond() / 3.0;
}
