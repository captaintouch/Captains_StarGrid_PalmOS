#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"

typedef struct GameSession {
    InputPen lastPenInput;
} GameSession;

GameSession gameSession;

void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_progressLogic();

#endif