#ifndef I_INPUT_H_
#define I_INPUT_H_

#include "../game/models.h"

typedef enum InputEventType {
    IEVENT_NONE,
    IEVENT_PENDOWN,
    IEVENT_PENMOVE,
    IEVENT_PENUP,
    IEVENT_MENU,
    IEVENT_BUTTON,
    IEVENT_DISPLAYCHANGED,
    IEVENT_APPSTOP,
    IEVENT_WINDOWEXIT,
    IEVENT_WINDOWENTER,
    IEVENT_UNHANDLED
} InputEventType;

typedef struct InputEvent {
    InputEventType type;
    Coordinate point;
    int id;  // menu item id / button id, depending on type
} InputEvent;

typedef void IRawEvent;

void iinput_translateEvent(IRawEvent *rawEvent, InputEvent *outEvent);
int iinput_isWindowExitingForm(IRawEvent *rawEvent, int formId);
int iinput_isWindowEnteringForm(IRawEvent *rawEvent, int formId);

#endif
