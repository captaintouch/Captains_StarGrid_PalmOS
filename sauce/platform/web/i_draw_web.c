#include "../i_draw.h"

#include <emscripten.h>
#include <string.h>

#include "web_bitmap.h"
#include "web_framebuffer.h"

/* Current drawing colors, packed RGB565. */
static unsigned short web_foreColor = 0xFFFF;
static unsigned short web_textColor = 0xFFFF;
static unsigned short web_backColor = 0x0000;

static WebBuffer *target() {
    return (web_currentTarget != NULL) ? web_currentTarget : web_displayBuffer;
}

static void plot(WebBuffer *b, int x, int y, unsigned short color) {
    if (b == NULL || x < 0 || y < 0 || x >= b->width || y >= b->height) {
        return;
    }
    b->pixels[y * b->width + x] = color;
}

IColorIndex idraw_indexForRGB(int red, int green, int blue) {
    int r = (red >> 3) & 0x1F;
    int g = (green >> 2) & 0x3F;
    int b = (blue >> 3) & 0x1F;
    return (IColorIndex)((r << 11) | (g << 5) | b);
}

void idraw_setForeColor(IColorIndex color) {
    web_foreColor = (unsigned short)color;
}

void idraw_setTextColor(IColorIndex color) {
    web_textColor = (unsigned short)color;
}

void idraw_setBackColor(IColorIndex color) {
    web_backColor = (unsigned short)color;
}

void idraw_fillRectangle(int x, int y, int width, int height, int cornerDiameter) {
    WebBuffer *b = target();
    int row, col;
    (void)cornerDiameter;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            plot(b, x + col, y + row, web_foreColor);
        }
    }
}

void idraw_frameRectangle(int x, int y, int width, int height) {
    WebBuffer *b = target();
    int i;
    for (i = 0; i < width; i++) {
        plot(b, x + i, y, web_foreColor);
        plot(b, x + i, y + height - 1, web_foreColor);
    }
    for (i = 0; i < height; i++) {
        plot(b, x, y + i, web_foreColor);
        plot(b, x + width - 1, y + i, web_foreColor);
    }
}

void idraw_drawLine(int x1, int y1, int x2, int y2) {
    WebBuffer *b = target();
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        plot(b, x1, y1, web_foreColor);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        {
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

void idraw_drawPixel(int x, int y) {
    plot(target(), x, y, web_foreColor);
}

/* Text is rendered by the browser's own font engine onto a transparent
   overlay <canvas> layered on top of the pixel canvas (see web/index.html
   and main_web.c's web_beginFrame/web_present), so it stays crisp at any
   pixel density instead of needing a baked-in bitmap font. */
EM_JS(void, web_jsDrawText, (const char *text, int x, int y, int color), {
    var ctx = Module.overlayCtx;
    if (!ctx) return;
    var str = UTF8ToString(text);
    var r = ((color >> 11) & 0x1F) * 255 / 31;
    var g = ((color >> 5) & 0x3F) * 255 / 63;
    var b = (color & 0x1F) * 255 / 31;
    ctx.fillStyle = "rgb(" + (r | 0) + "," + (g | 0) + "," + (b | 0) + ")";
    ctx.textBaseline = "top";
    ctx.font = "10px monospace";
    ctx.fillText(str, x, y);
});

EM_JS(int, web_jsTextWidth, (const char *text), {
    var ctx = Module.overlayCtx;
    if (!ctx) return UTF8ToString(text).length * 6;
    ctx.font = "10px monospace";
    return Math.ceil(ctx.measureText(UTF8ToString(text)).width);
});

void idraw_drawText(char *text, int x, int y) {
    if (text == NULL) {
        return;
    }
    web_jsDrawText(text, x, y, web_textColor);
}

void idraw_drawTextN(char *text, int length, int x, int y) {
    char buffer[256];
    int n = length;
    if (text == NULL) {
        return;
    }
    if (n >= (int)sizeof(buffer)) {
        n = sizeof(buffer) - 1;
    }
    memcpy(buffer, text, n);
    buffer[n] = '\0';
    web_jsDrawText(buffer, x, y, web_textColor);
}

int idraw_textWidth(char *text) {
    if (text == NULL) {
        return 0;
    }
    return web_jsTextWidth(text);
}

int idraw_textHeight() {
    return 10;
}

void idraw_drawBitmap(IBitmap *bitmap, int x, int y) {
    WebBitmap *wb = (WebBitmap *)bitmap;
    if (wb == NULL || wb->width <= 0 || wb->height <= 0) {
        return;
    }
    idraw_drawBitmapScaled(bitmap, x, y, wb->width, wb->height);
}

void idraw_drawBitmapScaled(IBitmap *bitmap, int x, int y, int width, int height) {
    WebBuffer *b = target();
    WebBitmap *wb = (WebBitmap *)bitmap;
    int row, col;
    if (wb == NULL || wb->width <= 0 || wb->height <= 0 || width <= 0 || height <= 0) {
        return;
    }
    for (row = 0; row < height; row++) {
        int srcRow = row * wb->height / height;
        for (col = 0; col < width; col++) {
            int srcCol = col * wb->width / width;
            int srcIndex = srcRow * wb->width + srcCol;
            if (wb->alpha[srcIndex] != 0) {
                plot(b, x + col, y + row, wb->color[srcIndex]);
            }
        }
    }
}

IFontID idraw_setLargeBoldFont() {
    return 0;
}

IFontID idraw_setFont(IFont font) {
    (void)font;
    return 0;
}

void idraw_restoreFont(IFontID font) {
    (void)font;
}
