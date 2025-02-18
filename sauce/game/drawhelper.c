#include "drawhelper.h"
#include "models.h"
#include "spriteLibrary.h"
#include <PalmOS.h>
#include "mathIsFun.h"

void drawhelper_fillRectangle(RectangleType *rect, UInt16 cornerDiam) {
    WinPaintRectangle(rect, cornerDiam);
}

void drawhelper_borderRectangle(RectangleType *rect) {
    WinDrawRectangleFrame(roundFrame, rect);
}

void drawhelper_drawLine(Line *line) {
    WinDrawLine(line->startpoint.x,
                line->startpoint.y,
                line->endpoint.x,
                line->endpoint.y);
}

void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint) {
    WinDrawLine(startpoint.x,
                startpoint.y,
                endpoint.x,
                endpoint.y);
}

void drawhelper_drawPoint(Coordinate point) {
    WinDrawPixel(point.x, point.y);
}

void drawhelper_applyForeColor(AppColor color) {
    WinSetForeColor(colors_reference[color]);
}

void drawhelper_applyTextColor(AppColor color) {
    WinSetTextColor(colors_reference[color]);
}

void drawhelper_applyBackgroundColor(AppColor color) {
    WinSetBackColor(colors_reference[color]);
}

void drawhelper_drawText(char *text, Coordinate position) {
    WinDrawChars(text, StrLen(text), position.x, position.y);
}

void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) {
    char finalText[20];
    char valueText[20];
    StrCopy(finalText, text);
    StrIToA(valueText, value);
    StrCat(finalText, valueText);
    WinDrawChars(finalText, StrLen(finalText), position.x, position.y);
}

static void drawhelper_drawImage(ImageData *imageData, Coordinate coordinate) {
    WinDrawBitmap(imageData->bitmapPtr, coordinate.x, coordinate.y);
}

void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate) {
    Coordinate updatedPosition;
    updatedPosition.x = coordinate.x - imageSprite->size.x / 2;
    updatedPosition.y = coordinate.y - imageSprite->size.y / 2;
    drawhelper_drawImage(imageSprite->imageData, updatedPosition);
}

// Returns true when animation has been finished
Boolean drawhelper_drawAnimatedSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, Int32 launchTimestamp, float durationSeconds) {
    float timePassedScale = (float)(TimGetTicks() - (float)launchTimestamp) / ((float)SysTicksPerSecond() * durationSeconds);
    int selectedIndex = fmin(frameCount - 1, timePassedScale * (float)(frameCount - 1));
    drawhelper_drawSprite(&imageSprite[selectedIndex], coordinate);
    return timePassedScale >= 1;
}

void drawhelper_drawBoxAround(Coordinate coordinate, int dimension) {
  Line line;
  int offset = dimension / 2 + 2;
  // TOP
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y - offset;
  drawhelper_drawLine(&line);
  // LEFT
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x - offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y + offset;
  drawhelper_drawLine(&line);
  // BOTTOM
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y + offset;
  line.endpoint.y = coordinate.y + offset;
  drawhelper_drawLine(&line);
  // RIGHT
  line.startpoint.x = coordinate.x + offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y + offset ;
  drawhelper_drawLine(&line);
}