#ifndef WEB_BITMAP_H_
#define WEB_BITMAP_H_

/* Stub bitmap handle shared by i_resource_web (creates them, tagged per
   sprite kind) and i_draw_web (draws them as simple filled shapes). This
   mirrors the CLI backend's single-representative-glyph approach but with a
   color + shape instead of an ASCII character. */
typedef struct WebBitmap {
    unsigned short color; /* RGB565 */
    int isCircle;
} WebBitmap;

#endif
