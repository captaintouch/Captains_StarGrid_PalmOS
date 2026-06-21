#include "i_ui.h"

#include <PalmOS.h>

IForm *iui_activeForm() {
    return (IForm *)FrmGetActiveForm();
}

void iui_deleteForm(IForm *form) {
    if (form != NULL) {
        FrmDeleteForm((FormType *)form);
    }
}

IForm *iui_newForm(int formId, int menuId, int width, int height) {
    FormType *form = FrmNewForm(formId, NULL, 0, 0, width, height, true, 0, 0, 0);
    if (menuId != 0) {
        FrmSetMenu(form, menuId);
    }
    return (IForm *)form;
}

void iui_setActiveForm(IForm *form) {
    FrmSetActiveForm((FormType *)form);
}

void iui_applyCustomDIAPolicy(IForm *form) {
    FrmSetDIAPolicyAttr((FormType *)form, frmDIAPolicyCustom);
    if (PINGetInputAreaState() != pinInputAreaClosed) {
        PINSetInputAreaState(pinInputAreaClosed);
    }
    if (PINGetInputTriggerState() != pinInputTriggerDisabled) {
        PINSetInputTriggerState(pinInputTriggerDisabled);
    }
}

int iui_activeFormId() {
    return FrmGetActiveFormID();
}

int iui_customAlert(int alertId, char *param1, char *param2) {
    return FrmCustomAlert(alertId, param1, param2, NULL);
}

void iui_showHelp(int helpStringId) {
    FrmHelp(helpStringId);
}

void iui_returnToForm(int formId) {
    FrmReturnToForm(formId);
}

void iui_initAndShowForm(int formId) {
    FormType *form = FrmInitForm(formId);
    FrmSetActiveForm(form);
    FrmDrawForm(form);
}
