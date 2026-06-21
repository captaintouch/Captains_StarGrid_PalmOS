#include "i_input.h"

#include <PalmOS.h>

void iinput_translateEvent(IRawEvent *rawEvent, InputEvent *outEvent) {
    EventType *event = (EventType *)rawEvent;
    outEvent->point = (Coordinate){0, 0};
    outEvent->id = 0;

    switch (event->eType) {
        case penDownEvent:
            outEvent->type = IEVENT_PENDOWN;
            outEvent->point = (Coordinate){event->screenX, event->screenY};
            break;
        case penMoveEvent:
            outEvent->type = IEVENT_PENMOVE;
            outEvent->point = (Coordinate){event->screenX, event->screenY};
            break;
        case penUpEvent:
            outEvent->type = IEVENT_PENUP;
            break;
        case menuEvent:
            outEvent->type = IEVENT_MENU;
            outEvent->id = event->data.menu.itemID;
            break;
        case ctlSelectEvent:
            outEvent->type = IEVENT_BUTTON;
            outEvent->id = event->data.ctlSelect.controlID;
            break;
        case winDisplayChangedEvent:
            outEvent->type = IEVENT_DISPLAYCHANGED;
            break;
        case appStopEvent:
            outEvent->type = IEVENT_APPSTOP;
            break;
        case winExitEvent:
            outEvent->type = IEVENT_WINDOWEXIT;
            break;
        case winEnterEvent:
            outEvent->type = IEVENT_WINDOWENTER;
            break;
        case nilEvent:
            outEvent->type = IEVENT_NONE;
            break;
        default:
            outEvent->type = IEVENT_UNHANDLED;
            break;
    }
}

int iinput_isWindowExitingForm(IRawEvent *rawEvent, int formId) {
    EventType *event = (EventType *)rawEvent;
    return event->eType == winExitEvent && event->data.winExit.exitWindow == (WinHandle)FrmGetFormPtr(formId);
}

int iinput_isWindowEnteringForm(IRawEvent *rawEvent, int formId) {
    EventType *event = (EventType *)rawEvent;
    return event->eType == winEnterEvent &&
           event->data.winEnter.enterWindow == (WinHandle)FrmGetFormPtr(formId) &&
           event->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm();
}
