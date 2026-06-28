#include "bottomMenu.h"
#include "models.h"
#include "../deviceinfo.h"
#include "../platform/i_draw.h"
#include "mathIsFun.h"
#include "drawhelper.h"

#define buttonHeight 17

static Coordinate bottomMenu_positionForButton(int index) {
    Coordinate screenSize = deviceinfo_screenSize();
    return (Coordinate){0, screenSize.y - index * buttonHeight - buttonHeight};
}

void bottomMenu_display(Button *buttons, Int8 buttonCount, Boolean colorSupport) {
    int i;
    RectangleType rect;
    Coordinate screenSize = deviceinfo_screenSize();
    IFontID oldFont = idraw_setLargeBoldFont();
    for (i = 0; i < buttonCount; i++) {
        Coordinate position = bottomMenu_positionForButton(i);
        AppColor bgColor;
        if (colorSupport) {
            bgColor = buttons[i].disabled ? DRACULAORCHID : ASBESTOS;
        } else {
            bgColor = buttons[i].disabled ? ASBESTOS : ALIZARIN;
        }
        if (i == 0) {
            bgColor = ALIZARIN;
        }
        

        RctSetRectangle(&rect, 0, position.y, screenSize.x, buttonHeight);

        drawhelper_applyTextColor(CLOUDS);

        drawhelper_applyBackgroundColor(bgColor);
        drawhelper_applyForeColor(bgColor);
        drawhelper_fillRectangle(&rect, 0);

        idraw_drawTextN(buttons[i].text, buttons[i].length, position.x + 4, position.y + 2);

        /* Raised-button bevel: light edge along the top, dark edge along the
           bottom, so flat-filled bars read as pressable chrome instead of
           plain colored stripes. */
        drawhelper_applyForeColor(CLOUDS);
        drawhelper_drawLineBetweenCoordinates((Coordinate){0, position.y}, (Coordinate){screenSize.x, position.y});
        drawhelper_applyForeColor(DRACULAORCHID);
        drawhelper_drawLineBetweenCoordinates((Coordinate){0, position.y + buttonHeight - 1}, (Coordinate){screenSize.x, position.y + buttonHeight - 1});

        drawhelper_applyForeColor(CLOUDS);
        drawhelper_drawLineBetweenCoordinates((Coordinate){0, position.y + buttonHeight}, (Coordinate){screenSize.x, position.y + buttonHeight});
    }
    idraw_restoreFont(oldFont);
}

Int8 bottomMenu_selectedIndex(Coordinate inputCoordinate) {
    int bottomOffset = deviceinfo_screenSize().y - inputCoordinate.y;
    return ceil(bottomOffset / buttonHeight);
}