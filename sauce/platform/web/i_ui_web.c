#include "../i_ui.h"

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * There is no real form/window system in the canvas shell. We only need to
 * track which "form" is active (the game checks
 * `iui_activeFormId() == GAME_FORM`), and surface alerts/help to the page.
 * Forms are represented by a heap int holding the form id (same approach as
 * the CLI backend).
 */

static int web_currentFormId = 0;
static IForm *web_activeForm = NULL;

IForm *iui_activeForm() {
    return web_activeForm;
}

void iui_deleteForm(IForm *form) {
    if (form == NULL) {
        return;
    }
    if (form == web_activeForm) {
        web_activeForm = NULL;
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
    web_activeForm = form;
    if (form != NULL) {
        web_currentFormId = *((int *)form);
    }
}

void iui_applyCustomDIAPolicy(IForm *form) {
    (void)form;
}

int iui_activeFormId() {
    return web_currentFormId;
}

int iui_customAlert(int alertId, char *param1, char *param2) {
    char message[256];
    snprintf(message, sizeof(message), "[%d] %s %s", alertId,
             (param1 != NULL) ? param1 : "", (param2 != NULL) ? param2 : "");
    EM_ASM({ alert(UTF8ToString($0)); }, message);
    return 0; /* first/affirmative button */
}

int iui_customAlert3(int alertId, char *param1, char *param2, char *param3) {
    char message[256];
    snprintf(message, sizeof(message), "[%d] %s %s %s", alertId,
             (param1 != NULL) ? param1 : "", (param2 != NULL) ? param2 : "",
             (param3 != NULL) ? param3 : "");
    EM_ASM({ alert(UTF8ToString($0)); }, message);
    return 0;
}

void iui_showHelp(int helpStringId) {
    fprintf(stderr, "[HELP %d]\n", helpStringId);
}

void iui_returnToForm(int formId) {
    web_currentFormId = formId;
}

void iui_initAndShowForm(int formId) {
    web_currentFormId = formId;
}
