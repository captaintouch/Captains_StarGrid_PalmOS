#include "i_draw.h"

#include <PalmOS.h>

IColorIndex idraw_indexForRGB(int red, int green, int blue) {
    RGBColorType color;
    color.r = red;
    color.g = green;
    color.b = blue;
    return WinRGBToIndex(&color);
}

void idraw_setForeColor(IColorIndex color) {
    WinSetForeColor(color);
}

void idraw_setTextColor(IColorIndex color) {
    WinSetTextColor(color);
}

void idraw_setBackColor(IColorIndex color) {
    WinSetBackColor(color);
}

void idraw_fillRectangle(int x, int y, int width, int height, int cornerDiameter) {
    RectangleType rect;
    RctSetRectangle(&rect, x, y, width, height);
    WinPaintRectangle(&rect, cornerDiameter);
}

void idraw_frameRectangle(int x, int y, int width, int height) {
    RectangleType rect;
    RctSetRectangle(&rect, x, y, width, height);
    WinDrawRectangleFrame(roundFrame, &rect);
}

void idraw_drawLine(int x1, int y1, int x2, int y2) {
    WinDrawLine(x1, y1, x2, y2);
}

void idraw_drawPixel(int x, int y) {
    WinDrawPixel(x, y);
}

void idraw_drawText(char *text, int x, int y) {
    WinDrawChars(text, StrLen(text), x, y);
}

void idraw_drawTextN(char *text, int length, int x, int y) {
    WinDrawChars(text, length, x, y);
}

IFontID idraw_setLargeBoldFont() {
    return FntSetFont(largeBoldFont);
}

void idraw_restoreFont(IFontID font) {
    FntSetFont(font);
}

int idraw_textWidth(char *text) {
    return FntCharsWidth(text, StrLen(text));
}

int idraw_textHeight() {
    return FntCharHeight();
}

void idraw_drawBitmap(IBitmap *bitmap, int x, int y) {
    WinDrawBitmap((BitmapPtr)bitmap, x, y);
}
