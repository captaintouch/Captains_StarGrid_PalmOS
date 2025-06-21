#ifndef DATABASE_H_
#define DATABASE_H_
#define DB_SECTION  __attribute__ ((section ("database")))
#include "game/scoring.h"

Score database_readScore() DB_SECTION;
void database_writeScore(Score *score) DB_SECTION;
#endif