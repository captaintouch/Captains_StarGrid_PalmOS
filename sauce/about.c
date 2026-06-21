#include "about.h"
#include "PalmTypes.h"
#include "constants.h"
#include "platform/i_ui.h"

Boolean about_buttonHandler(UInt16 buttonID) {
    if (iui_activeFormId() != ABOUT_FORM) {
        return false;
    }

    switch (buttonID) {
        case ABOUT_FORM_BUTTON_ACKNOWLEDGEMENTS:
            iui_customAlert(GAME_ALERT_ACKNOWLEDGMENTS, NULL, NULL);
            return true;
        default:
            iui_returnToForm(GAME_FORM);
            return true;
    }
}

void about_show() {
    iui_initAndShowForm(ABOUT_FORM);
}
