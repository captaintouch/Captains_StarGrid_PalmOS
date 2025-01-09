#ifndef BOTTOMMENU_H_
#define BOTTOMMENU_H_
#include <PalmOS.h>
#include "models.h"

typedef struct Button {
    char *text;
    Int8 length;
} Button;

void bottomMenu_display(Button *buttons, Int8 buttonCount);
Int8 bottomMenu_selectedIndex(Coordinate inputCoordinate);

#endif