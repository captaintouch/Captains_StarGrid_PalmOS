#ifndef INPUTPEN_H_
#define INPUTPEN_H_

#include <PalmOS.h>
#include "models.h"

typedef struct InputPen {
    Coordinate touchCoordinate;
    UInt32 blockUntilTicks;
    Boolean moving;
    Boolean penUpOccured;
    Boolean wasUpdatedFlag;
} InputPen;

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr);
void inputPen_temporarylyBlockPenInput(InputPen *pen);
#endif