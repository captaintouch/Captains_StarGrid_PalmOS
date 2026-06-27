#ifndef WEB_BITMAP_H_
#define WEB_BITMAP_H_

/* Bitmap handle shared by i_resource_web (resolves one per GFX_RES_* id,
   from the baked-in real art in web_assets_generated.h) and i_draw_web
   (blits it, nearest-neighbor scaled, onto the current target buffer). */
typedef struct WebBitmap {
    int width;
    int height;
    const unsigned short *color; /* RGB565, row-major */
    const unsigned char *alpha;  /* 0 or 255 per pixel */
} WebBitmap;

#endif
