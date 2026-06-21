#include "i_resource.h"

#include <PalmOS.h>

IBitmapHandle *iresource_loadBitmap(unsigned short bitmapId, void **outBitmapPtr) {
    MemHandle handle = DmGetResource(bitmapRsc, bitmapId);
    if (!handle) {
        return NULL;
    }
    *outBitmapPtr = (void *)MemHandleLock(handle);
    return (IBitmapHandle *)handle;
}

void iresource_releaseBitmap(IBitmapHandle *handle) {
    if (handle == NULL) {
        return;
    }
    MemHandleUnlock((MemHandle)handle);
    DmReleaseResource((MemHandle)handle);
}

char *iresource_loadString(unsigned short stringId, void **outHandle) {
    MemHandle handle = DmGetResource(strRsc, stringId);
    char *text;
    if (!handle) {
        if (outHandle != NULL) {
            *outHandle = NULL;
        }
        return NULL;
    }
    text = (char *)MemHandleLock(handle);
    if (outHandle != NULL) {
        *outHandle = (void *)handle;
    }
    return text;
}

void iresource_releaseString(void *handle) {
    if (handle == NULL) {
        return;
    }
    MemHandleUnlock((MemHandle)handle);
    DmReleaseResource((MemHandle)handle);
}
