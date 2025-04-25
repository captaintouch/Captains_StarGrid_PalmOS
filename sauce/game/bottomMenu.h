#ifndef BOTTOMMENU_H_
#define BOTTOMMENU_H_
#include <PalmOS.h>
#include "models.h"

typedef struct Button {
    char *text;
    Int8 length;
    Boolean disabled;
} Button;

void bottomMenu_display(Button *buttons, Int8 buttonCount, Boolean colorSupport);
Int8 bottomMenu_selectedIndex(Coordinate inputCoordinate);

#endif