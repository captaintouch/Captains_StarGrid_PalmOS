#ifndef I_STORAGE_H_
#define I_STORAGE_H_

typedef void IDatabase;

// Returns a DB id (opaque, > 0 if a database is found, 0 / negative if not found).
long istorage_findDatabase(char *name);
void istorage_deleteDatabase(long dbId);

// Creates a database with the given name/creator/type. Returns the new dbId, or 0 on failure.
long istorage_createDatabase(char *name, unsigned long creator, unsigned long type);

// Opens/closes a database for reading or writing.
IDatabase *istorage_openDatabase(long dbId, int forWriting);
void istorage_closeDatabase(IDatabase *db);

// Creates a new record of the given size in the database, writes data into it from src,
// and attaches it to the database (appended at the end). Returns 1 on success, 0 on failure.
int istorage_writeNewRecord(IDatabase *db, void *src, unsigned long size);

// Reads the record at the given index into dst (size bytes). Returns 1 on success, 0 if no record/failure.
int istorage_readRecord(IDatabase *db, int recordIndex, void *dst, unsigned long size);

// "Feature" blobs: arbitrary-sized named memory blocks that persist across launches (used for save-state).
int istorage_setFeatureBlob(unsigned long creator, unsigned short featureNum, void *src, unsigned long size);
void *istorage_getFeatureBlob(unsigned long creator, unsigned short featureNum);
void istorage_clearFeatureBlob(unsigned long creator, unsigned short featureNum);

#endif
