#ifndef CLI_FRAMEBUFFER_H_
#define CLI_FRAMEBUFFER_H_

/*
 * Shared in-memory ASCII framebuffer used by the CLI video and draw backends.
 * Each cell carries a glyph plus a foreground/background palette index (a small
 * ANSI color number 0-15). i_video_cli owns buffer lifetime and the current
 * draw target; i_draw_cli plots into whatever target is currently selected.
 */

typedef struct CliCell {
    char glyph;
    unsigned char fg;
    unsigned char bg;
} CliCell;

typedef struct CliBuffer {
    int width;
    int height;
    CliCell *cells;
} CliBuffer;

/* The buffer draw primitives currently target (NULL target maps to display). */
extern CliBuffer *cli_currentTarget;
/* The main on-screen buffer that gets printed to the terminal. */
extern CliBuffer *cli_displayBuffer;

CliBuffer *cli_bufferCreate(int width, int height);
void cli_bufferFree(CliBuffer *buffer);

/* Screen dimensions reported by i_device_cli (kept here so both agree). */
#define CLI_SCREEN_WIDTH 160
#define CLI_SCREEN_HEIGHT 160

#endif
