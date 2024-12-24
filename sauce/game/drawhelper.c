#include "drawhelper.h"
#include "models.h"
#include <PalmOS.h>

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

void drawhelper_applyForeColor(AppColor color) {
    WinSetForeColor(colors_reference[color]);
}

void drawhelper_applyTextColor(AppColor color) {
    WinSetTextColor(colors_reference[color]);
}

void drawhelper_applyBackgroundColor(AppColor color) {
    WinSetBackColor(colors_reference[color]);
}

void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) {
    char finalText[20];
    char valueText[20];
    StrCopy(finalText, text);
    StrIToA(valueText, value);
    StrCat(finalText, valueText);
    WinDrawChars(finalText, StrLen(finalText), position.x, position.y);
}