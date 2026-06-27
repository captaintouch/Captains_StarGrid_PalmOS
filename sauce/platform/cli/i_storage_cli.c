#include "../i_storage.h"

#include <stdlib.h>
#include <string.h>

/*
 * In-memory implementation of the storage interface. Persistence across launches
 * is not needed for the proof-of-concept, so databases and feature blobs live
 * only for the lifetime of the process.
 */

#define MAX_DATABASES 4
#define MAX_RECORDS 8
#define MAX_FEATURES 8

typedef struct CliRecord {
    void *data;
    unsigned long size;
} CliRecord;

typedef struct CliDatabase {
    int inUse;
    char name[64];
    CliRecord records[MAX_RECORDS];
    int recordCount;
} CliDatabase;

typedef struct CliFeature {
    int inUse;
    unsigned long creator;
    unsigned short featureNum;
    void *data;
    unsigned long size;
} CliFeature;

static CliDatabase databases[MAX_DATABASES];
static CliFeature features[MAX_FEATURES];

long istorage_findDatabase(char *name) {
    int i;
    for (i = 0; i < MAX_DATABASES; i++) {
        if (databases[i].inUse && strcmp(databases[i].name, name) == 0) {
            return i + 1; /* ids are 1-based; 0 means "not found" */
        }
    }
    return 0;
}

static void clearDatabaseRecords(CliDatabase *db) {
    int i;
    for (i = 0; i < db->recordCount; i++) {
        free(db->records[i].data);
        db->records[i].data = NULL;
        db->records[i].size = 0;
    }
    db->recordCount = 0;
}

void istorage_deleteDatabase(long dbId) {
    int index = (int)dbId - 1;
    if (index < 0 || index >= MAX_DATABASES || !databases[index].inUse) {
        return;
    }
    clearDatabaseRecords(&databases[index]);
    databases[index].inUse = 0;
}

long istorage_createDatabase(char *name, unsigned long creator, unsigned long type) {
    int i;
    (void)creator;
    (void)type;
    for (i = 0; i < MAX_DATABASES; i++) {
        if (!databases[i].inUse) {
            databases[i].inUse = 1;
            strncpy(databases[i].name, name, sizeof(databases[i].name) - 1);
            databases[i].name[sizeof(databases[i].name) - 1] = '\0';
            databases[i].recordCount = 0;
            return i + 1;
        }
    }
    return 0;
}

IDatabase *istorage_openDatabase(long dbId, int forWriting) {
    int index = (int)dbId - 1;
    (void)forWriting;
    if (index < 0 || index >= MAX_DATABASES || !databases[index].inUse) {
        return NULL;
    }
    return (IDatabase *)&databases[index];
}

void istorage_closeDatabase(IDatabase *db) {
    (void)db;
}

int istorage_writeNewRecord(IDatabase *db, void *src, unsigned long size) {
    CliDatabase *database = (CliDatabase *)db;
    void *copy;
    if (database == NULL || database->recordCount >= MAX_RECORDS) {
        return 0;
    }
    copy = malloc(size);
    if (copy == NULL) {
        return 0;
    }
    memcpy(copy, src, size);
    database->records[database->recordCount].data = copy;
    database->records[database->recordCount].size = size;
    database->recordCount++;
    return 1;
}

int istorage_readRecord(IDatabase *db, int recordIndex, void *dst, unsigned long size) {
    CliDatabase *database = (CliDatabase *)db;
    CliRecord *record;
    if (database == NULL || recordIndex < 0 || recordIndex >= database->recordCount) {
        return 0;
    }
    record = &database->records[recordIndex];
    if (record->data == NULL) {
        return 0;
    }
    memcpy(dst, record->data, (size < record->size) ? size : record->size);
    return 1;
}

static CliFeature *findFeature(unsigned long creator, unsigned short featureNum) {
    int i;
    for (i = 0; i < MAX_FEATURES; i++) {
        if (features[i].inUse && features[i].creator == creator && features[i].featureNum == featureNum) {
            return &features[i];
        }
    }
    return NULL;
}

int istorage_setFeatureBlob(unsigned long creator, unsigned short featureNum, void *src, unsigned long size) {
    CliFeature *feature = findFeature(creator, featureNum);
    int i;
    if (feature == NULL) {
        for (i = 0; i < MAX_FEATURES; i++) {
            if (!features[i].inUse) {
                feature = &features[i];
                break;
            }
        }
    }
    if (feature == NULL) {
        return 0;
    }
    free(feature->data);
    feature->data = malloc(size);
    if (feature->data == NULL) {
        feature->inUse = 0;
        return 0;
    }
    memcpy(feature->data, src, size);
    feature->size = size;
    feature->creator = creator;
    feature->featureNum = featureNum;
    feature->inUse = 1;
    return 1;
}

void *istorage_getFeatureBlob(unsigned long creator, unsigned short featureNum) {
    CliFeature *feature = findFeature(creator, featureNum);
    return (feature != NULL) ? feature->data : NULL;
}

void istorage_clearFeatureBlob(unsigned long creator, unsigned short featureNum) {
    CliFeature *feature = findFeature(creator, featureNum);
    if (feature == NULL) {
        return;
    }
    free(feature->data);
    feature->data = NULL;
    feature->size = 0;
    feature->inUse = 0;
}
