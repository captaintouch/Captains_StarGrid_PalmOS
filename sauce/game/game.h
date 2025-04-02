#ifndef GAME_H_
#define GAME_H_
#include <PalmOS.h>

typedef void (*openMainMenuCallback_t)();

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t callback);
void game_setup();
void game_cleanup();
int game_eventDelayTime();
#endif