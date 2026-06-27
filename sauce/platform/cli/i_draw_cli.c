#include "../i_draw.h"

#include "cli_framebuffer.h"

/* Current drawing colors (xterm-256 palette indices). */
static unsigned char cli_foreColor = 15;
static unsigned char cli_textColor = 15;
static unsigned char cli_backColor = 0;

static CliBuffer *target() {
    return (cli_currentTarget != NULL) ? cli_currentTarget : cli_displayBuffer;
}

static void plot(CliBuffer *b, int x, int y, char glyph, unsigned char fg, int setBg, unsigned char bg) {
    CliCell *cell;
    if (b == NULL || x < 0 || y < 0 || x >= b->width || y >= b->height) {
        return;
    }
    cell = &b->cells[y * b->width + x];
    cell->glyph = glyph;
    cell->fg = fg;
    if (setBg) {
        cell->bg = bg;
    }
}

IColorIndex idraw_indexForRGB(int red, int green, int blue) {
    /* Map to the xterm-256 6x6x6 color cube. */
    int r = (red * 6) / 256;
    int g = (green * 6) / 256;
    int b = (blue * 6) / 256;
    if (r > 5) r = 5;
    if (g > 5) g = 5;
    if (b > 5) b = 5;
    return (IColorIndex)(16 + 36 * r + 6 * g + b);
}

void idraw_setForeColor(IColorIndex color) {
    cli_foreColor = (unsigned char)color;
}

void idraw_setTextColor(IColorIndex color) {
    cli_textColor = (unsigned char)color;
}

void idraw_setBackColor(IColorIndex color) {
    cli_backColor = (unsigned char)color;
}

void idraw_fillRectangle(int x, int y, int width, int height, int cornerDiameter) {
    CliBuffer *b = target();
    int row, col;
    (void)cornerDiameter;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            plot(b, x + col, y + row, ' ', cli_foreColor, 1, cli_foreColor);
        }
    }
}

void idraw_frameRectangle(int x, int y, int width, int height) {
    CliBuffer *b = target();
    int i;
    for (i = 0; i < width; i++) {
        plot(b, x + i, y, '-', cli_foreColor, 0, 0);
        plot(b, x + i, y + height - 1, '-', cli_foreColor, 0, 0);
    }
    for (i = 0; i < height; i++) {
        plot(b, x, y + i, '|', cli_foreColor, 0, 0);
        plot(b, x + width - 1, y + i, '|', cli_foreColor, 0, 0);
    }
}

void idraw_drawLine(int x1, int y1, int x2, int y2) {
    CliBuffer *b = target();
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        plot(b, x1, y1, '*', cli_foreColor, 0, 0);
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
    plot(target(), x, y, '.', cli_foreColor, 0, 0);
}

void idraw_drawText(char *text, int x, int y) {
    CliBuffer *b = target();
    int i;
    if (text == NULL) {
        return;
    }
    for (i = 0; text[i] != '\0'; i++) {
        plot(b, x + i, y, text[i], cli_textColor, 0, 0);
    }
}

void idraw_drawTextN(char *text, int length, int x, int y) {
    CliBuffer *b = target();
    int i;
    if (text == NULL) {
        return;
    }
    for (i = 0; i < length && text[i] != '\0'; i++) {
        plot(b, x + i, y, text[i], cli_textColor, 0, 0);
    }
}

int idraw_textWidth(char *text) {
    int len = 0;
    if (text == NULL) {
        return 0;
    }
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

int idraw_textHeight() {
    return 1;
}

void idraw_drawBitmap(IBitmap *bitmap, int x, int y) {
    char glyph;
    if (bitmap == NULL) {
        return;
    }
    glyph = *((char *)bitmap);
    plot(target(), x, y, glyph, 15, 0, 0);
}

void idraw_drawBitmapScaled(IBitmap *bitmap, int x, int y, int width, int height) {
    /* A terminal cell can't be scaled, so the sprite stays a single glyph; draw
       it at the (scaled) top-left so it still lands on the right tile. */
    (void)width;
    (void)height;
    idraw_drawBitmap(bitmap, x, y);
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
