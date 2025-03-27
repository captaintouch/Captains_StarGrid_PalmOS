#ifndef INPUTPEN_H_
#define INPUTPEN_H_

#include <PalmOS.h>
#include "models.h"

typedef struct InputPen {
    Boolean moving;
    Boolean wasUpdatedFlag;
    Coordinate touchCoordinate;
} InputPen;

void inputPen_updateEventDetails(InputPen *pen, EventPtr eventPtr);

#endif