#include "../i_draw.h"

#include <emscripten.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../graphicResources.h"
#include "web_bitmap.h"
#include "web_framebuffer.h"
#include "web_text.h"

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

/* Blends src into the pixel at (x, y) by coverage (0..1), so diagonal hex
   edges get smooth anti-aliased steps instead of the harsh on/off stair-step
   a binary plot() produces. Web-only: the framebuffer is shared with Palm's
   line/bitmap interfaces only through i_draw.h, this implementation file is
   never compiled into the Palm build. */
static void plotBlended(WebBuffer *b, int x, int y, unsigned short color, float coverage) {
    unsigned short dst;
    int dr, dg, db, sr, sg, sb, r, g, bl;
    int alpha;
    if (b == NULL || x < 0 || y < 0 || x >= b->width || y >= b->height) {
        return;
    }
    if (coverage >= 0.999f) {
        b->pixels[y * b->width + x] = color;
        return;
    }
    if (coverage <= 0.001f) {
        return;
    }
    alpha = (int)(coverage * 255.0f);
    dst = b->pixels[y * b->width + x];
    dr = (dst >> 11) & 0x1F;
    dg = (dst >> 5) & 0x3F;
    db = dst & 0x1F;
    sr = (color >> 11) & 0x1F;
    sg = (color >> 5) & 0x3F;
    sb = color & 0x1F;
    r = (sr * alpha + dr * (255 - alpha)) / 255;
    g = (sg * alpha + dg * (255 - alpha)) / 255;
    bl = (sb * alpha + db * (255 - alpha)) / 255;
    b->pixels[y * b->width + x] = (unsigned short)((r << 11) | (g << 5) | bl);
}

/* Animated gradient/shimmer for the decorative hex tiles: a diagonal
   brightness band sweeps across the tile over time, giving the otherwise
   static, flat-colored tile art a subtle sense of motion. Confined to this
   web-only file (gated by GFX_RES_TILE* id) so it costs nothing on Palm,
   which never compiles this file and keeps drawing the plain bitmap. */
static Boolean isShimmeringTile(unsigned short resourceId) {
    return resourceId >= GFX_RES_TILEFEATURED && resourceId <= GFX_RES_TILEWARN;
}

static unsigned short applyShimmer(unsigned short color, int x, int y) {
    double phase = (x + y) * 0.25 - emscripten_get_now() * 0.003;
    float brightness = 1.0f + 0.22f * (float)sin(phase);
    int r = (color >> 11) & 0x1F;
    int g = (color >> 5) & 0x3F;
    int b = color & 0x1F;
    r = (int)(r * brightness);
    g = (int)(g * brightness);
    b = (int)(b * brightness);
    if (r > 0x1F) r = 0x1F;
    if (g > 0x3F) g = 0x3F;
    if (b > 0x1F) b = 0x1F;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return (unsigned short)((r << 11) | (g << 5) | b);
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

/* Xiaolin Wu's anti-aliased line algorithm: each step blends the two pixels
   straddling the ideal line by how close the line passes to each, instead of
   Bresenham's all-or-nothing stair-step. Horizontal/vertical lines (the
   common case for hex-tile borders' near-axis-aligned edges) skip straight
   to solid plots since there's nothing to anti-alias. */
static float fpart(float v) {
    return v - floorf(v);
}

void idraw_drawLine(int x1, int y1, int x2, int y2) {
    WebBuffer *b = target();
    Boolean steep = abs(y2 - y1) > abs(x2 - x1);
    int tmp;
    float dx, dy, gradient, intersectY;
    int x;

    if (x1 == x2 || y1 == y2) {
        int dxs = (x2 > x1) ? 1 : -1;
        int dys = (y2 > y1) ? 1 : -1;
        if (x1 == x2 && y1 == y2) {
            plot(b, x1, y1, web_foreColor);
            return;
        }
        while (1) {
            plot(b, x1, y1, web_foreColor);
            if (x1 == x2 && y1 == y2) {
                break;
            }
            if (x1 != x2) {
                x1 += dxs;
            }
            if (y1 != y2) {
                y1 += dys;
            }
        }
        return;
    }

    if (steep) {
        tmp = x1; x1 = y1; y1 = tmp;
        tmp = x2; x2 = y2; y2 = tmp;
    }
    if (x1 > x2) {
        tmp = x1; x1 = x2; x2 = tmp;
        tmp = y1; y1 = y2; y2 = tmp;
    }

    dx = (float)(x2 - x1);
    dy = (float)(y2 - y1);
    gradient = dy / dx;
    intersectY = y1;

    for (x = x1; x <= x2; x++) {
        int yi = (int)floorf(intersectY);
        float frac = fpart(intersectY);
        if (steep) {
            plotBlended(b, yi, x, web_foreColor, 1.0f - frac);
            plotBlended(b, yi + 1, x, web_foreColor, frac);
        } else {
            plotBlended(b, x, yi, web_foreColor, 1.0f - frac);
            plotBlended(b, x, yi + 1, web_foreColor, frac);
        }
        intersectY += gradient;
    }
}

void idraw_drawPixel(int x, int y) {
    plot(target(), x, y, web_foreColor);
}

/* Text is rendered by the browser's own font engine onto a transparent
   overlay <canvas> layered on top of the pixel canvas (see web/index.html
   and main_web.c's web_beginFrame/web_present), so it stays crisp at any
   pixel density instead of needing a baked-in bitmap font.

   "Press Start 2P" (loaded via <link> in index.html) gives the classic
   blocky bitmap-font look instead of the browser default monospace; a 1px
   dark drop-shadow behind the glyphs keeps labels legible over busy
   backgrounds, mimicking the outlined pixel-font text Apogee-era DOS
   titles used. */
EM_JS(void, web_jsDrawText, (const char *text, int x, int y, int color), {
    var ctx = Module.overlayCtx;
    if (!ctx) return;
    var str = UTF8ToString(text);
    var r = ((color >> 11) & 0x1F) * 255 / 31;
    var g = ((color >> 5) & 0x3F) * 255 / 63;
    var b = (color & 0x1F) * 255 / 31;
    ctx.textBaseline = "top";
    ctx.font = "8px 'Press Start 2P', monospace";
    ctx.fillStyle = "rgba(0,0,0,0.8)";
    ctx.fillText(str, x + 1, y + 1);
    ctx.fillStyle = "rgb(" + (r | 0) + "," + (g | 0) + "," + (b | 0) + ")";
    ctx.fillText(str, x, y);
});

EM_JS(int, web_jsTextWidth, (const char *text), {
    var ctx = Module.overlayCtx;
    if (!ctx) return UTF8ToString(text).length * 6;
    ctx.font = "8px 'Press Start 2P', monospace";
    return Math.ceil(ctx.measureText(UTF8ToString(text)).width);
});

void idraw_drawText(char *text, int x, int y) {
    if (text == NULL) {
        return;
    }
    web_textQueue(target(), x, y, text, web_textColor);
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
    web_textQueue(target(), x, y, buffer, web_textColor);
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
    Boolean shimmer;
    if (wb == NULL || wb->width <= 0 || wb->height <= 0 || width <= 0 || height <= 0) {
        return;
    }
    shimmer = isShimmeringTile(wb->resourceId);
    for (row = 0; row < height; row++) {
        /* Bilinearly sample the (binary) alpha mask at sub-pixel precision
           so upscaled sprite edges (notably the hex-tile fill shapes) get
           fractional coverage at the boundary instead of nearest-neighbor's
           blocky stair-step. Color is sampled nearest so tile colors stay
           crisp; only the edge coverage is smoothed. */
        float srcYf = (row + 0.5f) * wb->height / (float)height - 0.5f;
        int y0 = (int)floorf(srcYf);
        float fy = srcYf - y0;
        int y1 = y0 + 1;
        if (y0 < 0) y0 = 0;
        if (y1 < 0) y1 = 0;
        if (y0 > wb->height - 1) y0 = wb->height - 1;
        if (y1 > wb->height - 1) y1 = wb->height - 1;
        for (col = 0; col < width; col++) {
            float srcXf = (col + 0.5f) * wb->width / (float)width - 0.5f;
            int x0 = (int)floorf(srcXf);
            float fx = srcXf - x0;
            int x1 = x0 + 1;
            int a00, a10, a01, a11;
            float coverage;
            unsigned short nearestColor;
            int srcRow, srcCol;
            if (x0 < 0) x0 = 0;
            if (x1 < 0) x1 = 0;
            if (x0 > wb->width - 1) x0 = wb->width - 1;
            if (x1 > wb->width - 1) x1 = wb->width - 1;

            a00 = wb->alpha[y0 * wb->width + x0];
            a10 = wb->alpha[y0 * wb->width + x1];
            a01 = wb->alpha[y1 * wb->width + x0];
            a11 = wb->alpha[y1 * wb->width + x1];
            if (a00 == 0 && a10 == 0 && a01 == 0 && a11 == 0) {
                continue;
            }

            coverage = (a00 * (1 - fx) * (1 - fy) + a10 * fx * (1 - fy) + a01 * (1 - fx) * fy + a11 * fx * fy) / 255.0f;

            srcRow = row * wb->height / height;
            srcCol = col * wb->width / width;
            nearestColor = wb->color[srcRow * wb->width + srcCol];
            if (shimmer) {
                nearestColor = applyShimmer(nearestColor, x + col, y + row);
            }
            plotBlended(b, x + col, y + row, nearestColor, coverage);
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
