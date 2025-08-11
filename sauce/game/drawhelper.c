#include "drawhelper.h"

#include <PalmOS.h>

#include "../deviceinfo.h"
#include "mathIsFun.h"
#include "models.h"
#include "spriteLibrary.h"

DRAWING_SECTION
void drawhelper_fillRectangle(RectangleType *rect, UInt16 cornerDiam) {
    WinPaintRectangle(rect, cornerDiam);
}

DRAWING_SECTION
void drawhelper_fillRectangleWithShadow(RectangleType *rect, UInt16 cornerDiam, AppColor color, AppColor shadowColor, Boolean bottomRightShadow) {
    RectangleType shadowRect;
    if (bottomRightShadow) {
        RctSetRectangle(&shadowRect, rect->topLeft.x, rect->topLeft.y, rect->extent.x + 1, rect->extent.y + 1);
    } else {
        RctSetRectangle(&shadowRect, rect->topLeft.x - 1, rect->topLeft.y -1, rect->extent.x + 2, rect->extent.y + 2);
    }
    drawhelper_applyForeColor(shadowColor);
    WinPaintRectangle(&shadowRect, cornerDiam);
    drawhelper_applyForeColor(color);
    WinPaintRectangle(rect, cornerDiam);
}

DRAWING_SECTION
void drawhelper_drawCircle(Coordinate center, int radius) {
    RectangleType rect;
    RctSetRectangle(&rect, center.x - radius, center.y - radius, 2 * radius, 2 * radius);
    drawhelper_fillRectangle(&rect, radius);
}

DRAWING_SECTION
void drawhelper_borderRectangle(RectangleType *rect) {
    WinDrawRectangleFrame(roundFrame, rect);
}

DRAWING_SECTION
void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint) {
    WinDrawLine(startpoint.x,
                startpoint.y,
                endpoint.x,
                endpoint.y);
}

DRAWING_SECTION
void drawhelper_drawLine(Line *line) {
    drawhelper_drawLineBetweenCoordinates(line->startpoint, line->endpoint);
}

DRAWING_SECTION
void drawhelper_drawPoint(Coordinate point) {
    WinDrawPixel(point.x, point.y);
}

DRAWING_SECTION
void drawhelper_applyForeColor(AppColor color) {
    WinSetForeColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_applyTextColor(AppColor color) {
    WinSetTextColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_applyBackgroundColor(AppColor color) {
    WinSetBackColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_drawText(char *text, Coordinate position) {
    WinDrawChars(text, StrLen(text), position.x, position.y);
}

DRAWING_SECTION
void drawhelper_drawTextCentered(char *text, Coordinate position, int offsetX, int offsetY) {
    int width = FntCharsWidth(text, StrLen(text));
    WinDrawChars(text, StrLen(text), position.x - width / 2 + offsetX, position.y - FntCharHeight() / 2 + offsetY);
}

DRAWING_SECTION
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) {
    char finalText[20];
    char valueText[20];
    StrCopy(finalText, text);
    StrIToA(valueText, value);
    StrCat(finalText, valueText);
    WinDrawChars(finalText, StrLen(finalText), position.x, position.y);
}

DRAWING_SECTION
static void drawhelper_drawImage(ImageData *imageData, Coordinate coordinate) {
    Coordinate screenSize = deviceinfo_screenSize();
    if (coordinate.x > screenSize.x + 20 || coordinate.y > screenSize.y + 20 || coordinate.x < -20 || coordinate.y < -20) {
        return;
    }
    WinDrawBitmap(imageData->bitmapPtr, coordinate.x, coordinate.y);
}

DRAWING_SECTION
void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate) {
    Coordinate updatedPosition;
    updatedPosition.x = coordinate.x - imageSprite->size.x / 2;
    updatedPosition.y = coordinate.y - imageSprite->size.y / 2;
    drawhelper_drawImage(imageSprite->imageData, updatedPosition);
}

DRAWING_SECTION
void drawhelper_drawAnimatedSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, Int32 launchTimestamp, float durationSeconds) {
    float timePassedScale = (float)(TimGetTicks() - (float)launchTimestamp) / ((float)SysTicksPerSecond() * durationSeconds);
    int selectedIndex;
    if (timePassedScale >= 1) {
        return;
    }
    selectedIndex = fmin(frameCount - 1, timePassedScale * (float)(frameCount - 1));
    drawhelper_drawSprite(&imageSprite[selectedIndex], coordinate);
}

DRAWING_SECTION
void drawhelper_drawAnimatedLoopingSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, int animationsPerSecond, int frameSelectionOffset, int sleepingFrameCount) {
    int animationStep = ((TimGetTicks() / (SysTicksPerSecond() / animationsPerSecond)) + frameSelectionOffset) % (sleepingFrameCount + frameCount * 2);
    if (animationStep < sleepingFrameCount) {
        animationStep = 0;  // sleeping frame
    } else {
        animationStep -= sleepingFrameCount;
        if (animationStep >= frameCount) {
            animationStep = frameCount - 1 - (animationStep - frameCount);
        }
    }
    drawhelper_drawSprite(&imageSprite[animationStep], coordinate);
}

DRAWING_SECTION
void drawhelper_releaseImage(ImageData *imageData) {
    if (imageData == NULL) {
        return;
    }
    MemHandleUnlock(imageData->resource);
    DmReleaseResource(imageData->resource);
    MemPtrFree(imageData);
}

DRAWING_SECTION
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
    line.endpoint.y = coordinate.y + offset;
    drawhelper_drawLine(&line);
}
