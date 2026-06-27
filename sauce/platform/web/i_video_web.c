#include "../i_video.h"

#include <stdlib.h>

#include "../../game/models.h"
#include "../i_device.h"
#include "web_framebuffer.h"
#include "web_text.h"

WebBuffer *web_currentTarget = NULL;
WebBuffer *web_displayBuffer = NULL;

WebBuffer *web_bufferCreate(int width, int height) {
    WebBuffer *buffer = (WebBuffer *)malloc(sizeof(WebBuffer));
    int i;
    if (buffer == NULL) {
        return NULL;
    }
    buffer->width = width;
    buffer->height = height;
    buffer->pixels = (unsigned short *)malloc(sizeof(unsigned short) * width * height);
    if (buffer->pixels == NULL) {
        free(buffer);
        return NULL;
    }
    for (i = 0; i < width * height; i++) {
        buffer->pixels[i] = 0;
    }
    return buffer;
}

void web_bufferFree(WebBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(buffer->pixels);
    free(buffer);
}

static WebBuffer *web_ensureDisplay() {
    if (web_displayBuffer == NULL) {
        Coordinate size = idevice_screenSize();
        web_displayBuffer = web_bufferCreate(size.x, size.y);
    }
    return web_displayBuffer;
}

IVideoBuffer *ivideo_createBuffer(int width, int height) {
    return (IVideoBuffer *)web_bufferCreate(width, height);
}

void ivideo_deleteBuffer(IVideoBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    if (web_currentTarget == (WebBuffer *)buffer) {
        web_currentTarget = NULL;
    }
    web_bufferFree((WebBuffer *)buffer);
}

void ivideo_setDrawTarget(IVideoBuffer *buffer) {
    web_currentTarget = (buffer != NULL) ? (WebBuffer *)buffer : web_ensureDisplay();
}

IVideoBuffer *ivideo_mainScreenBuffer() {
    return (IVideoBuffer *)web_ensureDisplay();
}

void ivideo_copyRect(IVideoBuffer *src, IVideoBuffer *dst, Coordinate srcOrigin, Coordinate size, Coordinate dstOrigin) {
    WebBuffer *s = (WebBuffer *)src;
    WebBuffer *d = (WebBuffer *)dst;
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
            d->pixels[dy * d->width + dx] = s->pixels[sy * s->width + sx];
        }
    }
    web_textTranslate(s, d, srcOrigin.x, srcOrigin.y, size.x, size.y, dstOrigin.x, dstOrigin.y);
}
