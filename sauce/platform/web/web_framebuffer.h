#ifndef WEB_FRAMEBUFFER_H_
#define WEB_FRAMEBUFFER_H_

/*
 * Shared pixel framebuffer used by the web video and draw backends. Pixels are
 * packed RGB565 (16-bit) so IColorIndex can carry full color without a palette
 * table; main_web.c expands them to RGBA8888 once per frame for the canvas
 * blit. web_video_web owns buffer lifetime and the current draw target;
 * i_draw_web plots into whatever target is currently selected.
 */

typedef struct WebBuffer {
    int width;
    int height;
    unsigned short *pixels; /* RGB565 */
} WebBuffer;

extern WebBuffer *web_currentTarget;
extern WebBuffer *web_displayBuffer;

WebBuffer *web_bufferCreate(int width, int height);
void web_bufferFree(WebBuffer *buffer);

/* Default mobile-portrait canvas size; can be overridden before game_setup
   via web_setScreenSize (exported to JS) so the canvas can match the device. */
#define WEB_SCREEN_WIDTH 360
#define WEB_SCREEN_HEIGHT 640

#endif
