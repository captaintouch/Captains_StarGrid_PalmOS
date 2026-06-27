#include "web_text.h"

#include <string.h>

#define WEB_TEXT_MAX 256
#define WEB_TEXT_BUFLEN 64

typedef struct WebTextAnnotation {
    WebBuffer *target;
    int x, y;
    unsigned short color;
    char text[WEB_TEXT_BUFLEN];
} WebTextAnnotation;

static WebTextAnnotation web_textAnnotations[WEB_TEXT_MAX];
static int web_textCount = 0;

extern void web_jsDrawText(const char *text, int x, int y, int color);

void web_textQueue(WebBuffer *target, int x, int y, const char *text, unsigned short color) {
    WebTextAnnotation *entry;
    if (web_textCount >= WEB_TEXT_MAX || target == NULL || text == NULL) {
        return;
    }
    entry = &web_textAnnotations[web_textCount++];
    entry->target = target;
    entry->x = x;
    entry->y = y;
    entry->color = color;
    strncpy(entry->text, text, WEB_TEXT_BUFLEN - 1);
    entry->text[WEB_TEXT_BUFLEN - 1] = '\0';
}

void web_textTranslate(WebBuffer *src, WebBuffer *dst, int srcX, int srcY, int width, int height, int dstX, int dstY) {
    int i;
    for (i = 0; i < web_textCount; i++) {
        WebTextAnnotation *entry = &web_textAnnotations[i];
        if (entry->target != src) {
            continue;
        }
        if (entry->x < srcX || entry->x >= srcX + width || entry->y < srcY || entry->y >= srcY + height) {
            continue;
        }
        entry->target = dst;
        entry->x = dstX + (entry->x - srcX);
        entry->y = dstY + (entry->y - srcY);
    }
}

void web_textFlush(WebBuffer *target) {
    int i;
    for (i = 0; i < web_textCount; i++) {
        WebTextAnnotation *entry = &web_textAnnotations[i];
        if (entry->target == target) {
            web_jsDrawText(entry->text, entry->x, entry->y, entry->color);
        }
    }
}

void web_textClearAll(void) {
    web_textCount = 0;
}
