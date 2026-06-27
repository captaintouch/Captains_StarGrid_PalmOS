#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#include "../../constants.h"
#include "../../deviceinfo.h"
#include "../../game/colors.h"
#include "../../game/game.h"
#include "../../game/gamesession.h"

#include "cli_framebuffer.h"
#include "cli_input.h"

/* Optional scripted taps (from --tap X,Y[@FRAME] args) for reproducible demos. */
#define MAX_SCRIPTED_TAPS 16
typedef struct ScriptedTap {
    int x;
    int y;
    int atFrame;
} ScriptedTap;
static ScriptedTap scriptedTaps[MAX_SCRIPTED_TAPS];
static int scriptedTapCount = 0;

/* How densely the 160x160 internal screen is sampled when printed. */
#define ROW_STEP 2
#define COL_STEP 1
#define CURSOR_STEP 6

static struct termios cli_savedTermios;

static void cli_enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &cli_savedTermios);
    raw = cli_savedTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;  /* non-blocking read */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void cli_restoreMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &cli_savedTermios);
    printf("\033[0m\n");
    fflush(stdout);
}

static void cli_render(int cursorX, int cursorY) {
    int ry, rx;
    if (cli_displayBuffer == NULL) {
        return;
    }
    printf("\033[H");
    for (ry = 0; ry < cli_displayBuffer->height; ry += ROW_STEP) {
        for (rx = 0; rx < cli_displayBuffer->width; rx += COL_STEP) {
            CliCell cell = cli_displayBuffer->cells[ry * cli_displayBuffer->width + rx];
            char glyph = (cell.glyph != '\0') ? cell.glyph : ' ';
            int onCursor = (rx >= cursorX - COL_STEP && rx <= cursorX + COL_STEP &&
                            ry >= cursorY - ROW_STEP && ry <= cursorY + ROW_STEP);
            if (onCursor && glyph == ' ') {
                glyph = '+';
            }
            printf("\033[38;5;%dm\033[48;5;%dm%c", cell.fg, cell.bg, glyph);
        }
        printf("\033[0m\n");
    }
    printf("\033[0m");
    printf("Move: WASD  Tap: space  Menu:(x)exit (b)about (p)howto (r)reset  Quit:q   cursor=(%d,%d)   \n", cursorX, cursorY);
    printf("screen=%d turn=%d pawns=%d state=%d vp=(%d,%d)                  \n",
           gameSession.menuScreenType, gameSession.factionTurn, gameSession.level.pawnCount, gameSession.state,
           gameSession.viewportOffset.x, gameSession.viewportOffset.y);
    fflush(stdout);
}

static void cli_pumpKeys(int *cursorX, int *cursorY, int *running) {
    unsigned char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        switch (c) {
            case 'q':
                cli_input_push(CLI_RAW_APPSTOP, 0, 0, 0);
                *running = 0;
                break;
            case 'w':
                *cursorY -= CURSOR_STEP;
                break;
            case 's':
                *cursorY += CURSOR_STEP;
                break;
            case 'a':
                *cursorX -= CURSOR_STEP;
                break;
            case 'd':
                *cursorX += CURSOR_STEP;
                break;
            case ' ':
            case '\n':
            case '\r':
                cli_input_push(CLI_RAW_PENDOWN, *cursorX, *cursorY, 0);
                cli_input_push(CLI_RAW_PENUP, *cursorX, *cursorY, 0);
                break;
            case 'x':
                cli_input_push(CLI_RAW_MENU, 0, 0, GAME_MENUITEM_EXIT);
                break;
            case 'b':
                cli_input_push(CLI_RAW_MENU, 0, 0, GAME_MENUITEM_ABOUT);
                break;
            case 'p':
                cli_input_push(CLI_RAW_MENU, 0, 0, GAME_MENUITEM_HOWTOPLAY);
                break;
            case 'r':
                cli_input_push(CLI_RAW_MENU, 0, 0, GAME_MENUITEM_RESETRANK);
                break;
            default:
                break;
        }
    }
    if (*cursorX < 0) *cursorX = 0;
    if (*cursorY < 0) *cursorY = 0;
    if (*cursorX >= CLI_SCREEN_WIDTH) *cursorX = CLI_SCREEN_WIDTH - 1;
    if (*cursorY >= CLI_SCREEN_HEIGHT) *cursorY = CLI_SCREEN_HEIGHT - 1;
}

static void parseArgs(int argc, char **argv) {
    int i;
    for (i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--tap") == 0 && scriptedTapCount < MAX_SCRIPTED_TAPS) {
            int x = 0, y = 0, frame = 30;
            sscanf(argv[i + 1], "%d,%d@%d", &x, &y, &frame);
            scriptedTaps[scriptedTapCount].x = x;
            scriptedTaps[scriptedTapCount].y = y;
            scriptedTaps[scriptedTapCount].atFrame = frame;
            scriptedTapCount++;
            i++;
        }
    }
}

int main(int argc, char **argv) {
    int running = 1;
    int cursorX = CLI_SCREEN_WIDTH / 2;
    int cursorY = CLI_SCREEN_HEIGHT / 2;
    long frame = 0;
    int maxFrames = 0; /* 0 = run until 'q' */
    int i;
    struct timespec frameDelay;
    frameDelay.tv_sec = 0;
    frameDelay.tv_nsec = 40 * 1000000L; /* ~25 fps */

    parseArgs(argc, argv);
    for (i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--frames") == 0) {
            maxFrames = atoi(argv[i + 1]);
        }
    }

    srand((unsigned)time(NULL));
    cli_enableRawMode();
    colors_setupReferenceColors(deviceinfo_colorSupported(), deviceinfo_currentDepth());
    game_setup();

    printf("\033[2J");
    while (running) {
        CliRawEvent rawEvent;
        cli_pumpKeys(&cursorX, &cursorY, &running);

        for (i = 0; i < scriptedTapCount; i++) {
            if (scriptedTaps[i].atFrame == frame) {
                cli_input_push(CLI_RAW_PENDOWN, scriptedTaps[i].x, scriptedTaps[i].y, 0);
                cli_input_push(CLI_RAW_PENUP, scriptedTaps[i].x, scriptedTaps[i].y, 0);
            }
        }

        if (!cli_input_pop(&rawEvent)) {
            rawEvent.type = CLI_RAW_NIL;
            rawEvent.x = 0;
            rawEvent.y = 0;
            rawEvent.id = 0;
        }
        game_mainLoop(&rawEvent, NULL);
        cli_render(cursorX, cursorY);
        nanosleep(&frameDelay, NULL);
        frame++;
        if (maxFrames > 0 && frame >= maxFrames) {
            running = 0;
        }
    }

    game_cleanup();
    cli_restoreMode();
    return 0;
}
