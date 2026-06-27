#ifndef I_DRAW_H_
#define I_DRAW_H_

#include "../game/models.h"

typedef unsigned short IColorIndex;
typedef void IBitmap;
typedef int IFontID;

typedef enum IFont {
    IFONT_STD,
    IFONT_BOLD,
    IFONT_LARGEBOLD,
    IFONT_SYMBOL
} IFont;

IColorIndex idraw_indexForRGB(int red, int green, int blue);
void idraw_setForeColor(IColorIndex color);
void idraw_setTextColor(IColorIndex color);
void idraw_setBackColor(IColorIndex color);
void idraw_fillRectangle(int x, int y, int width, int height, int cornerDiameter);
void idraw_frameRectangle(int x, int y, int width, int height);
void idraw_drawLine(int x1, int y1, int x2, int y2);
void idraw_drawPixel(int x, int y);
void idraw_drawText(char *text, int x, int y);
void idraw_drawTextN(char *text, int length, int x, int y);
int idraw_textWidth(char *text);
int idraw_textHeight();
void idraw_drawBitmap(IBitmap *bitmap, int x, int y);
void idraw_drawBitmapScaled(IBitmap *bitmap, int x, int y, int width, int height);
IFontID idraw_setLargeBoldFont();
IFontID idraw_setFont(IFont font);
void idraw_restoreFont(IFontID font);

#endif
