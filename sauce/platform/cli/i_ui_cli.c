#include "../i_ui.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * There is no real form/window system on the terminal. We only need to track
 * which "form" is active (the game checks `iui_activeFormId() == GAME_FORM`),
 * and surface alerts/help as text. Forms are represented by a heap int holding
 * the form id.
 */

static int cli_currentFormId = 0;
static IForm *cli_activeForm = NULL;

IForm *iui_activeForm() {
    return cli_activeForm;
}

void iui_deleteForm(IForm *form) {
    if (form == NULL) {
        return;
    }
    if (form == cli_activeForm) {
        cli_activeForm = NULL;
    }
    free(form);
}

IForm *iui_newForm(int formId, int menuId, int width, int height) {
    int *form = (int *)malloc(sizeof(int));
    (void)menuId;
    (void)width;
    (void)height;
    if (form == NULL) {
        return NULL;
    }
    *form = formId;
    return (IForm *)form;
}

void iui_setActiveForm(IForm *form) {
    cli_activeForm = form;
    if (form != NULL) {
        cli_currentFormId = *((int *)form);
    }
}

void iui_applyCustomDIAPolicy(IForm *form) {
    (void)form;
}

int iui_activeFormId() {
    return cli_currentFormId;
}

int iui_customAlert(int alertId, char *param1, char *param2) {
    fprintf(stderr, "[ALERT %d] %s %s\n", alertId,
            (param1 != NULL) ? param1 : "",
            (param2 != NULL) ? param2 : "");
    return 0; /* first/affirmative button */
}

int iui_customAlert3(int alertId, char *param1, char *param2, char *param3) {
    fprintf(stderr, "[ALERT %d] %s %s %s\n", alertId,
            (param1 != NULL) ? param1 : "",
            (param2 != NULL) ? param2 : "",
            (param3 != NULL) ? param3 : "");
    return 0;
}

void iui_showHelp(int helpStringId) {
    fprintf(stderr, "[HELP %d]\n", helpStringId);
}

void iui_returnToForm(int formId) {
    cli_currentFormId = formId;
}

void iui_initAndShowForm(int formId) {
    cli_currentFormId = formId;
}
