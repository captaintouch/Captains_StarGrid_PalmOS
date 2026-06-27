#ifndef WEB_TEXT_H_
#define WEB_TEXT_H_

#include "web_framebuffer.h"

/*
 * Text is rendered onto a separate transparent overlay <canvas> (see
 * i_draw_web.c) instead of being plotted into a WebBuffer's pixel array, so
 * it stays crisp at any pixel density. But the pixel buffers go through a
 * chain of ivideo_copyRect translations (background -> overlay -> screen ->
 * mainWindow, e.g. game_drawLayout() in game.c shifts the overlay content
 * down by BOTTOMMENU_HEIGHT on non-game screens) before they end up on
 * screen. Text drawn straight onto the overlay canvas at its original,
 * untranslated coordinates would then land in the wrong place relative to
 * the pixel content around it.
 *
 * To keep text aligned with the pixel buffers it was drawn alongside, each
 * idraw_drawText/idraw_drawTextN call is queued here against the WebBuffer
 * it targeted instead of being drawn immediately. ivideo_copyRect re-targets
 * and translates any queued text that falls inside the copied region, the
 * same way the pixel copy does. Once a copy lands on web_displayBuffer, the
 * queued text is finally flushed to the JS canvas in main_web.c's per-frame
 * web_present call, then the queue is cleared for the next frame.
 */

void web_textQueue(WebBuffer *target, int x, int y, const char *text, unsigned short color);
void web_textTranslate(WebBuffer *src, WebBuffer *dst, int srcX, int srcY, int width, int height, int dstX, int dstY);
void web_textFlush(WebBuffer *target);
void web_textClearAll(void);

#endif
