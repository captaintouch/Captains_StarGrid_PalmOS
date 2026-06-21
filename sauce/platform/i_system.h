#ifndef I_SYSTEM_H_
#define I_SYSTEM_H_

unsigned long isys_getTicks();
unsigned long isys_ticksPerSecond();
int isys_random(int min, int max);
void isys_sleepMs(unsigned long milliseconds);
void isys_fatalError(char *message);
void isys_postOpenMenuEvent();

#endif
