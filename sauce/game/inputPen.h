#ifndef INPUTPEN_H_
#define INPUTPEN_H_

#include <PalmOS.h>
#include "models.h"

typedef struct InputPen {
    Coordinate touchCoordinate;
    Boolean moving;
    Boolean wasUpdatedFlag;
    Boolean blockUpdatesUntilPenUp;
} InputPen;

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr);

#endif