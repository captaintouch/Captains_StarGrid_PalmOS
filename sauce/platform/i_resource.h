#ifndef I_RESOURCE_H_
#define I_RESOURCE_H_

typedef void IBitmapHandle;

IBitmapHandle *iresource_loadBitmap(unsigned short bitmapId, void **outBitmapPtr);
void iresource_releaseBitmap(IBitmapHandle *handle);

char *iresource_loadString(unsigned short stringId, void **outHandle);
void iresource_releaseString(void *handle);

#endif
