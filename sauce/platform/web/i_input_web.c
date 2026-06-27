#include "../i_input.h"

#include <emscripten.h>

#include "web_input.h"

#define WEB_QUEUE_CAPACITY 64

static WebRawEvent web_queue[WEB_QUEUE_CAPACITY];
static int web_queueHead = 0;
static int web_queueTail = 0;

void web_input_push(WebRawType type, int x, int y, int id) {
    int next = (web_queueTail + 1) % WEB_QUEUE_CAPACITY;
    if (next == web_queueHead) {
        return; /* full, drop */
    }
    web_queue[web_queueTail].type = type;
    web_queue[web_queueTail].x = x;
    web_queue[web_queueTail].y = y;
    web_queue[web_queueTail].id = id;
    web_queueTail = next;
}

int web_input_pop(WebRawEvent *out) {
    if (web_queueHead == web_queueTail) {
        return 0;
    }
    *out = web_queue[web_queueHead];
    web_queueHead = (web_queueHead + 1) % WEB_QUEUE_CAPACITY;
    return 1;
}

void web_input_postMenu(void) {
    /* On Palm this would open the menu bar; the web shell has its own menu
       button (see web/index.html) so there's nothing to inject here. */
}

/* ---- Exported entry points called directly from JS event listeners ---- */

EMSCRIPTEN_KEEPALIVE
void web_pushPenDown(int x, int y) {
    web_input_push(WEB_RAW_PENDOWN, x, y, 0);
}

EMSCRIPTEN_KEEPALIVE
void web_pushPenMove(int x, int y) {
    web_input_push(WEB_RAW_PENMOVE, x, y, 0);
}

EMSCRIPTEN_KEEPALIVE
void web_pushPenUp(int x, int y) {
    web_input_push(WEB_RAW_PENUP, x, y, 0);
}

EMSCRIPTEN_KEEPALIVE
void web_pushMenu(int menuItemId) {
    web_input_push(WEB_RAW_MENU, 0, 0, menuItemId);
}

EMSCRIPTEN_KEEPALIVE
void web_pushAppStop(void) {
    web_input_push(WEB_RAW_APPSTOP, 0, 0, 0);
}

EMSCRIPTEN_KEEPALIVE
void web_pushDisplayChanged(void) {
    web_input_push(WEB_RAW_DISPLAYCHANGED, 0, 0, 0);
}

void iinput_translateEvent(IRawEvent *rawEvent, InputEvent *outEvent) {
    WebRawEvent *event = (WebRawEvent *)rawEvent;
    outEvent->point = (Coordinate){0, 0};
    outEvent->id = 0;

    switch (event->type) {
        case WEB_RAW_PENDOWN:
            outEvent->type = IEVENT_PENDOWN;
            outEvent->point = (Coordinate){event->x, event->y};
            break;
        case WEB_RAW_PENMOVE:
            outEvent->type = IEVENT_PENMOVE;
            outEvent->point = (Coordinate){event->x, event->y};
            break;
        case WEB_RAW_PENUP:
            outEvent->type = IEVENT_PENUP;
            break;
        case WEB_RAW_MENU:
            outEvent->type = IEVENT_MENU;
            outEvent->id = event->id;
            break;
        case WEB_RAW_BUTTON:
            outEvent->type = IEVENT_BUTTON;
            outEvent->id = event->id;
            break;
        case WEB_RAW_APPSTOP:
            outEvent->type = IEVENT_APPSTOP;
            break;
        case WEB_RAW_DISPLAYCHANGED:
            outEvent->type = IEVENT_DISPLAYCHANGED;
            break;
        case WEB_RAW_NIL:
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
