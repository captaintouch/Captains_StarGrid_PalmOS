#ifndef WEB_INPUT_H_
#define WEB_INPUT_H_

/*
 * Web raw input event + a tiny FIFO queue, mirroring the CLI backend's
 * approach. JS touch/pointer/menu-button listeners call the exported
 * web_push* functions (see i_input_web.c) to enqueue raw events; i_input_web
 * turns them into the portable InputEvent the game understands.
 */

typedef enum WebRawType {
    WEB_RAW_NIL,
    WEB_RAW_PENDOWN,
    WEB_RAW_PENMOVE,
    WEB_RAW_PENUP,
    WEB_RAW_MENU,
    WEB_RAW_BUTTON,
    WEB_RAW_APPSTOP,
    WEB_RAW_DISPLAYCHANGED
} WebRawType;

typedef struct WebRawEvent {
    WebRawType type;
    int x;
    int y;
    int id;
} WebRawEvent;

void web_input_push(WebRawType type, int x, int y, int id);
int web_input_pop(WebRawEvent *out); /* 1 if an event was popped, 0 if empty */
void web_input_postMenu(void);

#endif
