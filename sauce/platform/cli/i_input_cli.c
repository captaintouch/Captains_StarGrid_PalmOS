#include "../i_input.h"

#include "cli_input.h"

#define CLI_QUEUE_CAPACITY 64

static CliRawEvent cli_queue[CLI_QUEUE_CAPACITY];
static int cli_queueHead = 0;
static int cli_queueTail = 0;

void cli_input_push(CliRawType type, int x, int y, int id) {
    int next = (cli_queueTail + 1) % CLI_QUEUE_CAPACITY;
    if (next == cli_queueHead) {
        return; /* full, drop */
    }
    cli_queue[cli_queueTail].type = type;
    cli_queue[cli_queueTail].x = x;
    cli_queue[cli_queueTail].y = y;
    cli_queue[cli_queueTail].id = id;
    cli_queueTail = next;
}

int cli_input_pop(CliRawEvent *out) {
    if (cli_queueHead == cli_queueTail) {
        return 0;
    }
    *out = cli_queue[cli_queueHead];
    cli_queueHead = (cli_queueHead + 1) % CLI_QUEUE_CAPACITY;
    return 1;
}

void cli_input_postMenu(void) {
    /* On Palm this would open the menu bar; in the CLI the user opens menu
       actions with dedicated keys, so nothing to inject here. */
}

void iinput_translateEvent(IRawEvent *rawEvent, InputEvent *outEvent) {
    CliRawEvent *event = (CliRawEvent *)rawEvent;
    outEvent->point = (Coordinate){0, 0};
    outEvent->id = 0;

    switch (event->type) {
        case CLI_RAW_PENDOWN:
            outEvent->type = IEVENT_PENDOWN;
            outEvent->point = (Coordinate){event->x, event->y};
            break;
        case CLI_RAW_PENMOVE:
            outEvent->type = IEVENT_PENMOVE;
            outEvent->point = (Coordinate){event->x, event->y};
            break;
        case CLI_RAW_PENUP:
            outEvent->type = IEVENT_PENUP;
            break;
        case CLI_RAW_MENU:
            outEvent->type = IEVENT_MENU;
            outEvent->id = event->id;
            break;
        case CLI_RAW_BUTTON:
            outEvent->type = IEVENT_BUTTON;
            outEvent->id = event->id;
            break;
        case CLI_RAW_APPSTOP:
            outEvent->type = IEVENT_APPSTOP;
            break;
        case CLI_RAW_DISPLAYCHANGED:
            outEvent->type = IEVENT_DISPLAYCHANGED;
            break;
        case CLI_RAW_NIL:
        default:
            outEvent->type = IEVENT_NONE;
            break;
    }
}

int iinput_isWindowExitingForm(IRawEvent *rawEvent, int formId) {
    (void)rawEvent;
    (void)formId;
    return 0;
}

int iinput_isWindowEnteringForm(IRawEvent *rawEvent, int formId) {
    (void)rawEvent;
    (void)formId;
    return 0;
}
