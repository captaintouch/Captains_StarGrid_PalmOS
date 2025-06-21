#include "database.h"

#include <PalmOS.h>

#include "game/scoring.h"
#include "constants.h"

#define DB_TYPE 'DATA'
#define DBSCORE_NAME "StarGrid ScoreDB"

DB_SECTION
static LocalID database_scoreDatabaseId() {
    return DmFindDatabase(0, DBSCORE_NAME);
}

DB_SECTION
static LocalID database_recreateScoreDatabase() {
    LocalID dbId;
    DmOpenRef dbRef;
    Err error;

    dbId = database_scoreDatabaseId();
    if (dbId > 0) {
        DmDeleteDatabase(0, dbId);
    }

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
void database_writeScore(Score *score) {
    DmOpenRef dbRef;
    UInt16 recordIndex;
    MemHandle recordHandle;
    Err err;
    Score *packed;
    UInt32 recordSize;

    LocalID dbId = database_recreateScoreDatabase();
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
Score database_readScore() {
    LocalID dbId;
    DmOpenRef dbRef;
    MemHandle recordHandle;
    Score *packed;
    Score score;

    MemSet(&score, sizeof(Score), 0);
    dbId = database_scoreDatabaseId();
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