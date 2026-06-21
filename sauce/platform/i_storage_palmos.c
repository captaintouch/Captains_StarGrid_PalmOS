#include "i_storage.h"

#include <PalmOS.h>

long istorage_findDatabase(char *name) {
    return (long)DmFindDatabase(0, name);
}

void istorage_deleteDatabase(long dbId) {
    if (dbId > 0) {
        DmDeleteDatabase(0, (LocalID)dbId);
    }
}

long istorage_createDatabase(char *name, unsigned long creator, unsigned long type) {
    LocalID dbId;
    DmOpenRef dbRef;
    Err error = DmCreateDatabase(0, name, creator, type, false);
    if (error != errNone) {
        return 0;
    }
    dbId = DmFindDatabase(0, name);
    if (!dbId) {
        return 0;
    }
    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!dbRef) {
        return 0;
    }
    DmCloseDatabase(dbRef);
    return (long)dbId;
}

IDatabase *istorage_openDatabase(long dbId, int forWriting) {
    return (IDatabase *)DmOpenDatabase(0, (LocalID)dbId, forWriting ? dmModeReadWrite : dmModeReadOnly);
}

void istorage_closeDatabase(IDatabase *db) {
    if (db != NULL) {
        DmCloseDatabase((DmOpenRef)db);
    }
}

int istorage_writeNewRecord(IDatabase *db, void *src, unsigned long size) {
    UInt16 recordIndex;
    Err err;
    void *packed;
    MemHandle recordHandle = DmNewHandle((DmOpenRef)db, size);
    if (recordHandle == NULL) {
        return 0;
    }
    packed = MemHandleLock(recordHandle);
    DmWrite(packed, 0, src, size);
    MemHandleUnlock(recordHandle);
    recordIndex = dmMaxRecordIndex;
    err = DmAttachRecord((DmOpenRef)db, &recordIndex, recordHandle, NULL);
    return err == errNone;
}

int istorage_readRecord(IDatabase *db, int recordIndex, void *dst, unsigned long size) {
    void *packed;
    MemHandle recordHandle = DmQueryRecord((DmOpenRef)db, recordIndex);
    if (!recordHandle) {
        return 0;
    }
    packed = MemHandleLock(recordHandle);
    if (!packed) {
        return 0;
    }
    MemMove(dst, packed, size);
    MemHandleUnlock(recordHandle);
    return 1;
}

int istorage_setFeatureBlob(unsigned long creator, unsigned short featureNum, void *src, unsigned long size) {
    void *ptr;
    if (FtrPtrNew(creator, featureNum, size, &ptr) != errNone) {
        return 0;
    }
    DmWrite(ptr, 0, src, size);
    return 1;
}

void *istorage_getFeatureBlob(unsigned long creator, unsigned short featureNum) {
    UInt32 ptr;
    if (FtrGet(creator, featureNum, &ptr) != errNone) {
        return NULL;
    }
    return (void *)ptr;
}

void istorage_clearFeatureBlob(unsigned long creator, unsigned short featureNum) {
    FtrUnregister(creator, featureNum);
}
