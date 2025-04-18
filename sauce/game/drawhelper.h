#ifndef DRAWHELPER_H_
#define DRAWHELPER_H_
#include <PalmOS.h>
#include "models.h"
#include "colors.h"
#include "spriteLibrary.h"

void drawhelper_fillRectangle(RectangleType *rect, UInt16 cornerDiam);
void drawhelper_fillRectangleWithShadow(RectangleType *rect, UInt16 cornerDiam, AppColor color, AppColor shadowColor);
void drawhelper_drawCircle(Coordinate center, int radius);
void drawhelper_drawLine(Line *line);
void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint);
void drawhelper_applyForeColor(AppColor color);
void drawhelper_applyTextColor(AppColor color);
void drawhelper_applyBackgroundColor(AppColor color);
void drawhelper_drawText(char *text, Coordinate position);
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position);
void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate);
void drawhelper_drawAnimatedSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, Int32 launchTimestamp, float durationSeconds);
void drawhelper_drawPoint(Coordinate point);
void drawhelper_drawBoxAround(Coordinate coordinate, int dimension);
void drawhelper_borderRectangle(RectangleType *rect);
void drawhelper_releaseImage(ImageData *imageData);

#endif