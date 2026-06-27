#include "drawhelper.h"

#include "../platform/i_draw.h"
#include "../platform/i_system.h"
#include "../platform/i_memory.h"
#include "../platform/i_resource.h"

#include "../deviceinfo.h"
#include "mathIsFun.h"
#include "models.h"
#include "spriteLibrary.h"

/* Sprite zoom, as a percentage of native size. 100 = no scaling, which uses the
   exact historical draw path so unscaled platforms render identically. */
static int drawhelper_spriteScalePercent = 100;

void drawhelper_setSpriteScale(int percent) {
    drawhelper_spriteScalePercent = percent > 0 ? percent : 100;
}

DRAWING_SECTION
void drawhelper_fillRectangle(RectangleType *rect, UInt16 cornerDiam) {
    idraw_fillRectangle(rect->topLeft.x, rect->topLeft.y, rect->extent.x, rect->extent.y, cornerDiam);
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
    idraw_fillRectangle(shadowRect.topLeft.x, shadowRect.topLeft.y, shadowRect.extent.x, shadowRect.extent.y, cornerDiam);
    drawhelper_applyForeColor(color);
    idraw_fillRectangle(rect->topLeft.x, rect->topLeft.y, rect->extent.x, rect->extent.y, cornerDiam);
}

DRAWING_SECTION
void drawhelper_drawCircle(Coordinate center, int radius) {
    RectangleType rect;
    RctSetRectangle(&rect, center.x - radius, center.y - radius, 2 * radius, 2 * radius);
    drawhelper_fillRectangle(&rect, radius);
}

DRAWING_SECTION
void drawhelper_borderRectangle(RectangleType *rect) {
    idraw_frameRectangle(rect->topLeft.x, rect->topLeft.y, rect->extent.x, rect->extent.y);
}

DRAWING_SECTION
void drawhelper_drawLineBetweenCoordinates(Coordinate startpoint, Coordinate endpoint) {
    idraw_drawLine(startpoint.x, startpoint.y, endpoint.x, endpoint.y);
}

DRAWING_SECTION
void drawhelper_drawLine(Line *line) {
    drawhelper_drawLineBetweenCoordinates(line->startpoint, line->endpoint);
}

DRAWING_SECTION
void drawhelper_drawPoint(Coordinate point) {
    idraw_drawPixel(point.x, point.y);
}

DRAWING_SECTION
void drawhelper_applyForeColor(AppColor color) {
    idraw_setForeColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_applyTextColor(AppColor color) {
    idraw_setTextColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_applyBackgroundColor(AppColor color) {
    idraw_setBackColor(colors_reference[color]);
}

DRAWING_SECTION
void drawhelper_drawText(char *text, Coordinate position) {
    idraw_drawText(text, position.x, position.y);
}

DRAWING_SECTION
void drawhelper_drawTextCentered(char *text, Coordinate position, int offsetX, int offsetY) {
    int width = idraw_textWidth(text);
    idraw_drawText(text, position.x - width / 2 + offsetX, position.y - idraw_textHeight() / 2 + offsetY);
}

DRAWING_SECTION
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) {
    char finalText[20];
    char valueText[20];
    StrCopy(finalText, text);
    StrIToA(valueText, value);
    StrCat(finalText, valueText);
    idraw_drawText(finalText, position.x, position.y);
}

DRAWING_SECTION
static void drawhelper_drawImage(ImageData *imageData, Coordinate coordinate) {
    Coordinate screenSize = deviceinfo_screenSize();
    if (coordinate.x > screenSize.x + 20 || coordinate.y > screenSize.y + 20 || coordinate.x < -20 || coordinate.y < -20) {
        return;
    }
    idraw_drawBitmap(imageData->bitmapPtr, coordinate.x, coordinate.y);
}

DRAWING_SECTION
void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate) {
    Coordinate updatedPosition;
    if (drawhelper_spriteScalePercent == 100) {
        /* Unscaled: identical to the historical path. */
        updatedPosition.x = coordinate.x - imageSprite->size.x / 2;
        updatedPosition.y = coordinate.y - imageSprite->size.y / 2;
        drawhelper_drawImage(imageSprite->imageData, updatedPosition);
    } else {
        Coordinate screenSize = deviceinfo_screenSize();
        int width = imageSprite->size.x * drawhelper_spriteScalePercent / 100;
        int height = imageSprite->size.y * drawhelper_spriteScalePercent / 100;
        updatedPosition.x = coordinate.x - width / 2;
        updatedPosition.y = coordinate.y - height / 2;
        if (updatedPosition.x > screenSize.x + 20 || updatedPosition.y > screenSize.y + 20 || updatedPosition.x < -width - 20 || updatedPosition.y < -height - 20) {
            return;
        }
        idraw_drawBitmapScaled(imageSprite->imageData->bitmapPtr, updatedPosition.x, updatedPosition.y, width, height);
    }
}

DRAWING_SECTION
void drawhelper_drawAnimatedSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, Int32 launchTimestamp, float durationSeconds) {
    float timePassedScale = (float)(isys_getTicks() - (float)launchTimestamp) / ((float)isys_ticksPerSecond() * durationSeconds);
    int selectedIndex;
    if (timePassedScale >= 1) {
        return;
    }
    selectedIndex = fmin(frameCount - 1, timePassedScale * (float)(frameCount - 1));
    drawhelper_drawSprite(&imageSprite[selectedIndex], coordinate);
}

DRAWING_SECTION
void drawhelper_drawAnimatedLoopingSprite(ImageSprite *imageSprite, UInt8 frameCount, Coordinate coordinate, int animationsPerSecond, int frameSelectionOffset, int sleepingFrameCount) {
    int animationStep = ((isys_getTicks() / (isys_ticksPerSecond() / animationsPerSecond)) + frameSelectionOffset) % (sleepingFrameCount + frameCount * 2);
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
    iresource_releaseBitmap(imageData->resourceHandle);
    imem_free(imageData);
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
