#include "bottomMenu.h"
#include "models.h"
#include "../deviceinfo.h"
#include "mathIsFun.h"
#include "drawhelper.h"

#define buttonHeight 17

static Coordinate bottomMenu_positionForButton(int index) {
    Coordinate screenSize = deviceinfo_screenSize();
    return (Coordinate){0, screenSize.y - index * buttonHeight - buttonHeight};
}

void bottomMenu_display(Button *buttons, Int8 buttonCount) {
    int i;
    RectangleType rect;
    Coordinate screenSize = deviceinfo_screenSize();
    FontID oldFont = FntSetFont(largeBoldFont);
    for (i = 0; i < buttonCount; i++) {
        Coordinate position = bottomMenu_positionForButton(i);
        AppColor bgColor = ASBESTOS;
        if (i == 0) {
            bgColor = ALIZARIN;
        }
        

        RctSetRectangle(&rect, 0, position.y, screenSize.x, buttonHeight);
        
        drawhelper_applyTextColor(CLOUDS);

        drawhelper_applyBackgroundColor(bgColor);
        drawhelper_applyForeColor(bgColor);
        drawhelper_fillRectangle(&rect, 0);

        WinDrawChars(buttons[i].text, buttons[i].length, position.x + 4, position.y + 2);

        drawhelper_applyForeColor(CLOUDS);
        drawhelper_drawLineBetweenCoordinates((Coordinate){0, position.y + buttonHeight}, (Coordinate){screenSize.x, position.y + buttonHeight});
    }
    FntSetFont(oldFont);
}

Int8 bottomMenu_selectedIndex(Coordinate inputCoordinate) {
    int bottomOffset = deviceinfo_screenSize().y - inputCoordinate.y;
    return ceil(bottomOffset / buttonHeight);
}