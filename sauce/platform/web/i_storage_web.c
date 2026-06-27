#include "../i_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web_localstorage.h"

/*
 * In-memory database/feature-blob state (same shape as the CLI backend),
 * with write-through persistence to the browser's localStorage so save
 * state and the score record survive a page reload - still no backend
 * service, localStorage lives entirely in the browser.
 */

#define MAX_DATABASES 4
#define MAX_RECORDS 8
#define MAX_FEATURES 8
#define MAX_BLOB_SIZE 4096

typedef struct WebRecord {
    void *data;
    unsigned long size;
} WebRecord;

typedef struct WebDatabase {
    int inUse;
    char name[64];
    WebRecord records[MAX_RECORDS];
    int recordCount;
} WebDatabase;

typedef struct WebFeature {
    int inUse;
    unsigned long creator;
    unsigned short featureNum;
    void *data;
    unsigned long size;
} WebFeature;

static WebDatabase databases[MAX_DATABASES];
static WebFeature features[MAX_FEATURES];

static void dbStorageKey(char *out, int outSize, const char *name) {
    snprintf(out, outSize, "sg_db_%s", name);
}

static void featureStorageKey(char *out, int outSize, unsigned long creator, unsigned short featureNum) {
    snprintf(out, outSize, "sg_feat_%lu_%u", creator, (unsigned int)featureNum);
}

/* Serializes a database's records into a flat blob: recordCount, then
   [size,bytes] per record. Kept simple since records here are small fixed
   structs (one Score per save). */
static void persistDatabase(WebDatabase *db) {
    char key[96];
    unsigned char blob[MAX_BLOB_SIZE];
    unsigned long offset = 0;
    int i;
    int count = db->recordCount;
    dbStorageKey(key, sizeof(key), db->name);
    memcpy(blob + offset, &count, sizeof(int));
    offset += sizeof(int);
    for (i = 0; i < db->recordCount; i++) {
        unsigned long size = db->records[i].size;
        if (offset + sizeof(unsigned long) + size > MAX_BLOB_SIZE) {
            break; /* shouldn't happen for this game's record sizes */
        }
        memcpy(blob + offset, &size, sizeof(unsigned long));
        offset += sizeof(unsigned long);
        memcpy(blob + offset, db->records[i].data, size);
        offset += size;
    }
    web_ls_save(key, blob, (int)offset);
}

static void loadDatabaseFromStorage(WebDatabase *db) {
    char key[96];
    unsigned char blob[MAX_BLOB_SIZE];
    unsigned long offset = 0;
    int count, i, length;
    dbStorageKey(key, sizeof(key), db->name);
    length = web_ls_load(key, blob, sizeof(blob));
    if (length < (int)sizeof(int)) {
        return;
    }
    memcpy(&count, blob + offset, sizeof(int));
    offset += sizeof(int);
    for (i = 0; i < count && i < MAX_RECORDS; i++) {
        unsigned long size;
        memcpy(&size, blob + offset, sizeof(unsigned long));
        offset += sizeof(unsigned long);
        db->records[i].data = malloc(size);
        if (db->records[i].data != NULL) {
            memcpy(db->records[i].data, blob + offset, size);
            db->records[i].size = size;
        }
        offset += size;
    }
    db->recordCount = (count < MAX_RECORDS) ? count : MAX_RECORDS;
}

static WebDatabase *findOrLoadDatabase(char *name) {
    int i;
    for (i = 0; i < MAX_DATABASES; i++) {
        if (databases[i].inUse && strcmp(databases[i].name, name) == 0) {
            return &databases[i];
        }
    }
    {
        char key[96];
        unsigned char probe[4];
        dbStorageKey(key, sizeof(key), name);
        if (web_ls_load(key, probe, sizeof(probe)) < 0) {
            return NULL; /* not persisted, and not already open */
        }
    }
    for (i = 0; i < MAX_DATABASES; i++) {
        if (!databases[i].inUse) {
            databases[i].inUse = 1;
            strncpy(databases[i].name, name, sizeof(databases[i].name) - 1);
            databases[i].name[sizeof(databases[i].name) - 1] = '\0';
            databases[i].recordCount = 0;
            loadDatabaseFromStorage(&databases[i]);
            return &databases[i];
        }
    }
    return NULL;
}

long istorage_findDatabase(char *name) {
    WebDatabase *db = findOrLoadDatabase(name);
    if (db == NULL) {
        return 0;
    }
    return (long)(db - databases) + 1; /* ids are 1-based; 0 means "not found" */
}

static void clearDatabaseRecords(WebDatabase *db) {
    int i;
    for (i = 0; i < db->recordCount; i++) {
        free(db->records[i].data);
        db->records[i].data = NULL;
        db->records[i].size = 0;
    }
    db->recordCount = 0;
}

void istorage_deleteDatabase(long dbId) {
    char key[96];
    int index = (int)dbId - 1;
    if (index < 0 || index >= MAX_DATABASES || !databases[index].inUse) {
        return;
    }
    dbStorageKey(key, sizeof(key), databases[index].name);
    web_ls_remove(key);
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
            persistDatabase(&databases[i]);
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
    WebDatabase *database = (WebDatabase *)db;
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
    persistDatabase(database);
    return 1;
}

int istorage_readRecord(IDatabase *db, int recordIndex, void *dst, unsigned long size) {
    WebDatabase *database = (WebDatabase *)db;
    WebRecord *record;
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

static WebFeature *findFeature(unsigned long creator, unsigned short featureNum) {
    int i;
    for (i = 0; i < MAX_FEATURES; i++) {
        if (features[i].inUse && features[i].creator == creator && features[i].featureNum == featureNum) {
            return &features[i];
        }
    }
    return NULL;
}

static WebFeature *loadFeatureFromStorage(unsigned long creator, unsigned short featureNum) {
    char key[64];
    unsigned char blob[MAX_BLOB_SIZE];
    int length, i;
    featureStorageKey(key, sizeof(key), creator, featureNum);
    length = web_ls_load(key, blob, sizeof(blob));
    if (length < 0) {
        return NULL;
    }
    for (i = 0; i < MAX_FEATURES; i++) {
        if (!features[i].inUse) {
            features[i].data = malloc(length);
            if (features[i].data == NULL) {
                return NULL;
            }
            memcpy(features[i].data, blob, length);
            features[i].size = length;
            features[i].creator = creator;
            features[i].featureNum = featureNum;
            features[i].inUse = 1;
            return &features[i];
        }
    }
    return NULL;
}

int istorage_setFeatureBlob(unsigned long creator, unsigned short featureNum, void *src, unsigned long size) {
    char key[64];
    WebFeature *feature = findFeature(creator, featureNum);
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
    featureStorageKey(key, sizeof(key), creator, featureNum);
    web_ls_save(key, src, (int)size);
    return 1;
}

void *istorage_getFeatureBlob(unsigned long creator, unsigned short featureNum) {
    WebFeature *feature = findFeature(creator, featureNum);
    if (feature == NULL) {
        feature = loadFeatureFromStorage(creator, featureNum);
    }
    return (feature != NULL) ? feature->data : NULL;
}

void istorage_clearFeatureBlob(unsigned long creator, unsigned short featureNum) {
    char key[64];
    WebFeature *feature = findFeature(creator, featureNum);
    featureStorageKey(key, sizeof(key), creator, featureNum);
    web_ls_remove(key);
    if (feature == NULL) {
        return;
    }
    free(feature->data);
    feature->data = NULL;
    feature->size = 0;
    feature->inUse = 0;
}
