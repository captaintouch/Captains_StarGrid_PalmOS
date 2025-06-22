#include "about.h"
#include <PalmOS.h>
#include "constants.h"

Boolean about_buttonHandler(UInt16 buttonID) {
    if (FrmGetActiveFormID() != ABOUT_FORM) {
        return false;
    }

    switch (buttonID) {
        case ABOUT_FORM_BUTTON_ACKNOWLEDGEMENTS:
            FrmCustomAlert(GAME_ALERT_ACKNOWLEDGMENTS, NULL, NULL, NULL);
            return true;
        default:
            FrmReturnToForm(GAME_FORM);
            return true;
    }
}

void about_show() {
    FormType *frmP = FrmInitForm(ABOUT_FORM);
    FrmSetActiveForm(frmP);
    FrmDrawForm(frmP);
}
