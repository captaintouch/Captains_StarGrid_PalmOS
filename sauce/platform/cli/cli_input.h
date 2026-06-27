#ifndef CLI_INPUT_H_
#define CLI_INPUT_H_

/*
 * CLI raw input event + a tiny FIFO queue. main_cli translates keystrokes into
 * these raw events; i_input_cli turns them into the portable InputEvent the game
 * understands. This mirrors how the Palm backend turns an EventType into an
 * InputEvent, except the "raw" type here is our own struct rather than Palm's.
 */

typedef enum CliRawType {
    CLI_RAW_NIL,
    CLI_RAW_PENDOWN,
    CLI_RAW_PENMOVE,
    CLI_RAW_PENUP,
    CLI_RAW_MENU,
    CLI_RAW_BUTTON,
    CLI_RAW_APPSTOP,
    CLI_RAW_DISPLAYCHANGED
} CliRawType;

typedef struct CliRawEvent {
    CliRawType type;
    int x;
    int y;
    int id;
} CliRawEvent;

void cli_input_push(CliRawType type, int x, int y, int id);
int cli_input_pop(CliRawEvent *out); /* 1 if an event was popped, 0 if empty */
void cli_input_postMenu(void);

#endif
