#ifndef INPUTPEN_H_
#define INPUTPEN_H_

#include "../platform/i_input.h"
#include "models.h"

typedef struct InputPen {
    Coordinate touchCoordinate;
    UInt32 blockUntilTicks;
    Boolean moving;
    Boolean penUpOccured;
    Boolean wasUpdatedFlag;
} InputPen;

void inputPen_updateEventDetails(InputPen *pen, InputEvent *event);
void inputPen_temporarylyBlockPenInput(InputPen *pen);
#endif
