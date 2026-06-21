#ifndef I_UI_H_
#define I_UI_H_

typedef void IForm;

IForm *iui_activeForm();
void iui_deleteForm(IForm *form);
IForm *iui_newForm(int formId, int menuId, int width, int height);
void iui_setActiveForm(IForm *form);
void iui_applyCustomDIAPolicy(IForm *form);
int iui_activeFormId();
int iui_customAlert(int alertId, char *param1, char *param2);
void iui_showHelp(int helpStringId);
void iui_returnToForm(int formId);
void iui_initAndShowForm(int formId);

#endif
