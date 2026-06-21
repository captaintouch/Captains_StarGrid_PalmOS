#ifndef GAME_H_
#define GAME_H_

#include "../platform/i_input.h"

typedef void (*openMainMenuCallback_t)();

Boolean game_mainLoop(IRawEvent *rawEvent, openMainMenuCallback_t callback);
void game_setup();
void game_cleanup();
int game_eventDelayTime();
#endif
