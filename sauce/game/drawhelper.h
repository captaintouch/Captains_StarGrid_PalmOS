#ifndef DRAWHELPER_H_
#define DRAWHELPER_H_
#include <PalmOS.h>
#include "models.h"
#include "colors.h"

void drawhelper_drawLine(Line *line);
void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint);
void drawhelper_applyForeColor(AppColor color);
void drawhelper_applyTextColor(AppColor color);
void drawhelper_applyBackgroundColor(AppColor color);
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position);
#endif