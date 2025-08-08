#ifndef DRAWHELPER_H_
#define DRAWHELPER_H_
#define DRAWING_SECTION  __attribute__ ((section ("drawing")))
#include <PalmOS.h>
#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"

void drawhelper_fillRectangle(RectangleType *rect, UInt16 cornerDiam) DRAWING_SECTION;
void drawhelper_fillRectangleWithShadow(RectangleType *rect, UInt16 cornerDiam, AppColor color, AppColor shadowColor) DRAWING_SECTION;
void drawhelper_drawCircle(Coordinate center, int radius) DRAWING_SECTION;
void drawhelper_drawLine(Line *line) DRAWING_SECTION;
void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint) DRAWING_SECTION;
void drawhelper_applyForeColor(AppColor color) DRAWING_SECTION;
void drawhelper_applyTextColor(AppColor color) DRAWING_SECTION;
void drawhelper_applyBackgroundColor(AppColor color) DRAWING_SECTION;
void drawhelper_drawText(char *text, Coordinate position) DRAWING_SECTION;
void drawhelper_drawTextCentered(char *text, Coordinate position, int offsetX, int offsetY) DRAWING_SECTION;
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) DRAWING_SECTION;
void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate) DRAWING_SECTION;
void drawhelper_drawAnimatedSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, Int32 launchTimestamp, float durationSeconds) DRAWING_SECTION;
void drawhelper_drawAnimatedLoopingSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, int animationsPerSecond) DRAWING_SECTION; 
void drawhelper_drawPoint(Coordinate point) DRAWING_SECTION;
void drawhelper_drawBoxAround(Coordinate coordinate, int dimension) DRAWING_SECTION;
void drawhelper_borderRectangle(RectangleType *rect) DRAWING_SECTION;
void drawhelper_releaseImage(ImageData *imageData) DRAWING_SECTION;

#endif
