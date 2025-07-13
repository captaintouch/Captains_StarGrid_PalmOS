#include "storage.h"

#include <PalmOS.h>

#include "game/scoring.h"
#include "constants.h"
#include "game/models.h"

#define DB_TYPE 'DATA'
#define DBSCORE_NAME "StarGrid ScoreDB"

DB_SECTION
static LocalID storage_scoreDatabaseId() {
    return DmFindDatabase(0, DBSCORE_NAME);
}

DB_SECTION
void storage_reset() {
    LocalID dbId = dbId = storage_scoreDatabaseId();
    if (dbId > 0) {
        DmDeleteDatabase(0, dbId);
    }
}

DB_SECTION
static LocalID storage_recreateScoreDatabase() {
    LocalID dbId;
    DmOpenRef dbRef;
    Err error;

    storage_reset();

    error = DmCreateDatabase(0, DBSCORE_NAME, APP_CREATOR_ID, DB_TYPE, false);
    if (error != errNone) {
        return 0;
    }

    dbId = DmFindDatabase(0, DBSCORE_NAME);
    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!dbRef) {
        return 0;
    }
    DmCloseDatabase(dbRef);
    return dbId;
}

DB_SECTION
void storage_writeScore(Score *score) {
    DmOpenRef dbRef;
    UInt16 recordIndex;
    MemHandle recordHandle;
    Err err;
    Score *packed;
    UInt32 recordSize;

    LocalID dbId = storage_recreateScoreDatabase();
    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    recordSize = sizeof(Score);

    recordHandle = DmNewHandle(dbRef, recordSize);
    if (recordHandle == NULL) {
        return;
    }

    packed = (Score *)MemHandleLock(recordHandle);

    DmWrite(packed, 0, score, sizeof(Score));

    MemHandleUnlock(recordHandle);

    recordIndex = dmMaxRecordIndex;

    err = DmAttachRecord(dbRef, &recordIndex, recordHandle, NULL);
    if (err != errNone) {
        return;
    }
    
    DmCloseDatabase(dbRef);
    
}

DB_SECTION
Score storage_readScore() {
    LocalID dbId;
    DmOpenRef dbRef;
    MemHandle recordHandle;
    Score *packed;
    Score score;

    MemSet(&score, sizeof(Score), 0);
    dbId = storage_scoreDatabaseId();
    if (dbId <= 0) {
        return score;
    }

    dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly);
    if (!dbRef) {
        return score;
    }

    recordHandle = DmQueryRecord(dbRef, 0);
    if (!recordHandle) {
        DmCloseDatabase(dbRef);
        return score;
    }

    packed = MemHandleLock(recordHandle);
    if (!packed) {
        DmCloseDatabase(dbRef);
        return score;
    }
    
    MemMove(&score, packed, sizeof(Score));
    MemHandleUnlock(recordHandle);
    DmCloseDatabase(dbRef);
    return score;
}

DB_SECTION
static void storage_clearSavedGameState() {
    FtrUnregister(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA);
    FtrUnregister(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS);
}

DB_SECTION
void storage_saveGameState(int currentTurn, int pawnCount, int factionCount, int factionTurn, Pawn *pawns, Faction factions[GAMEMECHANICS_MAXPLAYERCOUNT], LevelScore scores[GAMEMECHANICS_MAXPLAYERCOUNT]) {
    void *sessionDataPtr;
    void *pawnDataPtr;
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

    FtrPtrNew(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA, sizeof(GameRestorableSessionData), &sessionDataPtr);
    FtrPtrNew(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS, sizeof(Pawn) * pawnCount, &pawnDataPtr);
    DmWrite(sessionDataPtr, 0, &sessionData, sizeof(GameRestorableSessionData));
    DmWrite(pawnDataPtr, 0, pawns, sizeof(Pawn) * pawnCount);
}

DB_SECTION
Boolean storage_restoreGameState(UInt8 *currentTurn, int *pawnCount, int *factionCount, int *factionTurn, Pawn **pawns, Faction *factions, LevelScore *scores) {
    UInt32 sessionDataPtr, pawnDataPtr;
    GameRestorableSessionData *sessionData;
    Pawn *pawnData;
    int i;
    if (FtrGet(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA, &sessionDataPtr) != errNone || FtrGet(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS, &pawnDataPtr) != errNone) {
        storage_clearSavedGameState();
        return false;
    }

    sessionData = (GameRestorableSessionData *)sessionDataPtr;
    pawnData = (Pawn *)pawnDataPtr;

    *currentTurn = sessionData->currentTurn;
    *factionCount = sessionData->factionCount;
    *factionTurn = sessionData->factionTurn;
    for (i = 0; i < GAMEMECHANICS_MAXPLAYERCOUNT; i++) {
        factions[i] = sessionData->factions[i];
        scores[i] = sessionData->scores[i];
    }

    *pawnCount = sessionData->pawnCount;
    *pawns = (Pawn *)MemPtrNew(sizeof(Pawn) * sessionData->pawnCount);
    MemSet(*pawns, sizeof(Pawn) * sessionData->pawnCount, 0);
    for (i = 0; i < sessionData->pawnCount; i++) {
        MemMove(&(*pawns)[i], &pawnData[i], sizeof(Pawn));
    }
    storage_clearSavedGameState();
    return true;
}