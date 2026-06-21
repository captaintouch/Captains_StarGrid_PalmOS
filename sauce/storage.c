#include "storage.h"

#include "platform/i_storage.h"
#include "platform/i_memory.h"

#include "game/scoring.h"
#include "constants.h"
#include "game/models.h"

#define DB_TYPE 'DATA'
#define DBSCORE_NAME "StarGrid ScoreDB"

STORAGE_SECTION
static long storage_scoreDatabaseId() {
    return istorage_findDatabase(DBSCORE_NAME);
}

STORAGE_SECTION
void storage_reset() {
    long dbId = storage_scoreDatabaseId();
    if (dbId > 0) {
        istorage_deleteDatabase(dbId);
    }
}

STORAGE_SECTION
static long storage_recreateScoreDatabase() {
    storage_reset();
    return istorage_createDatabase(DBSCORE_NAME, APP_CREATOR_ID, DB_TYPE);
}

STORAGE_SECTION
void storage_writeScore(Score *score) {
    IDatabase *db;
    long dbId = storage_recreateScoreDatabase();
    if (dbId == 0) {
        return;
    }
    db = istorage_openDatabase(dbId, 1);
    if (!db) {
        return;
    }
    istorage_writeNewRecord(db, score, sizeof(Score));
    istorage_closeDatabase(db);
}

STORAGE_SECTION
Score storage_readScore() {
    long dbId;
    IDatabase *db;
    Score score;

    imem_zero(&score, sizeof(Score));
    dbId = storage_scoreDatabaseId();
    if (dbId <= 0) {
        return score;
    }

    db = istorage_openDatabase(dbId, 0);
    if (!db) {
        return score;
    }

    istorage_readRecord(db, 0, &score, sizeof(Score));
    istorage_closeDatabase(db);
    return score;
}

STORAGE_SECTION
static void storage_clearSavedGameState() {
    istorage_clearFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA);
    istorage_clearFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS);
}

STORAGE_SECTION
void storage_saveGameState(int currentTurn, int pawnCount, int factionCount, int factionTurn, Pawn *pawns, Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT], LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT]) {
    int i;
    GameRestorableSessionData sessionData;
    storage_clearSavedGameState();

    sessionData.currentTurn = currentTurn;
    sessionData.pawnCount = pawnCount;
    sessionData.factionCount = factionCount;
    sessionData.factionTurn = factionTurn;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        sessionData.factions[i] = factions[i];
        sessionData.scores[i] = scores[i];
    }

    istorage_setFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA, &sessionData, sizeof(GameRestorableSessionData));
    istorage_setFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS, pawns, sizeof(Pawn) * pawnCount);
}

STORAGE_SECTION
Boolean storage_restoreGameState(UInt8 *currentTurn, int *pawnCount, int *factionCount, int *factionTurn, Pawn **pawns, Faction *factions, LevelScore *scores) {
    GameRestorableSessionData *sessionData;
    Pawn *pawnData;
    int i;

    sessionData = (GameRestorableSessionData *)istorage_getFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA);
    pawnData = (Pawn *)istorage_getFeatureBlob(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS);
    if (sessionData == NULL || pawnData == NULL) {
        storage_clearSavedGameState();
        return false;
    }

    *currentTurn = sessionData->currentTurn;
    *factionCount = sessionData->factionCount;
    *factionTurn = sessionData->factionTurn;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        factions[i] = sessionData->factions[i];
        scores[i] = sessionData->scores[i];
    }

    *pawnCount = sessionData->pawnCount;
    *pawns = (Pawn *)imem_alloc(sizeof(Pawn) * sessionData->pawnCount);
    imem_zero(*pawns, sizeof(Pawn) * sessionData->pawnCount);
    for (i = 0; i < sessionData->pawnCount; i++) {
        imem_copy(&(*pawns)[i], &pawnData[i], sizeof(Pawn));
    }
    storage_clearSavedGameState();
    return true;
}
