#include "../i_video.h"

#include <stdlib.h>

#include "cli_framebuffer.h"

CliBuffer *cli_currentTarget = NULL;
CliBuffer *cli_displayBuffer = NULL;

CliBuffer *cli_bufferCreate(int width, int height) {
    CliBuffer *buffer = (CliBuffer *)malloc(sizeof(CliBuffer));
    int i;
    if (buffer == NULL) {
        return NULL;
    }
    buffer->width = width;
    buffer->height = height;
    buffer->cells = (CliCell *)malloc(sizeof(CliCell) * width * height);
    if (buffer->cells == NULL) {
        free(buffer);
        return NULL;
    }
    for (i = 0; i < width * height; i++) {
        buffer->cells[i].glyph = ' ';
        buffer->cells[i].fg = 7;
        buffer->cells[i].bg = 0;
    }
    return buffer;
}

void cli_bufferFree(CliBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(buffer->cells);
    free(buffer);
}

static CliBuffer *cli_ensureDisplay() {
    if (cli_displayBuffer == NULL) {
        cli_displayBuffer = cli_bufferCreate(CLI_SCREEN_WIDTH, CLI_SCREEN_HEIGHT);
    }
    return cli_displayBuffer;
}

IVideoBuffer *ivideo_createBuffer(int width, int height) {
    return (IVideoBuffer *)cli_bufferCreate(width, height);
}

void ivideo_deleteBuffer(IVideoBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    if (cli_currentTarget == (CliBuffer *)buffer) {
        cli_currentTarget = NULL;
    }
    cli_bufferFree((CliBuffer *)buffer);
}

void ivideo_setDrawTarget(IVideoBuffer *buffer) {
    cli_currentTarget = (buffer != NULL) ? (CliBuffer *)buffer : cli_ensureDisplay();
}

IVideoBuffer *ivideo_mainScreenBuffer() {
    return (IVideoBuffer *)cli_ensureDisplay();
}

void ivideo_copyRect(IVideoBuffer *src, IVideoBuffer *dst, Coordinate srcOrigin, Coordinate size, Coordinate dstOrigin) {
    CliBuffer *s = (CliBuffer *)src;
    CliBuffer *d = (CliBuffer *)dst;
    int row, col;
    if (s == NULL || d == NULL) {
        return;
    }
    for (row = 0; row < size.y; row++) {
        int sy = srcOrigin.y + row;
        int dy = dstOrigin.y + row;
        if (sy < 0 || sy >= s->height || dy < 0 || dy >= d->height) {
            continue;
        }
        for (col = 0; col < size.x; col++) {
            int sx = srcOrigin.x + col;
            int dx = dstOrigin.x + col;
            if (sx < 0 || sx >= s->width || dx < 0 || dx >= d->width) {
                continue;
            }
            d->cells[dy * d->width + dx] = s->cells[sy * s->width + sx];
        }
    }
}
