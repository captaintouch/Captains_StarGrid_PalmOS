#ifndef I_VIDEO_H_
#define I_VIDEO_H_

#include "../game/models.h"

typedef void IVideoBuffer;

IVideoBuffer *ivideo_createBuffer(int width, int height);
void ivideo_deleteBuffer(IVideoBuffer *buffer);
void ivideo_setDrawTarget(IVideoBuffer *buffer);  // NULL = main screen
IVideoBuffer *ivideo_mainScreenBuffer();
void ivideo_copyRect(IVideoBuffer *src, IVideoBuffer *dst, Coordinate srcOrigin, Coordinate size, Coordinate dstOrigin);

#endif
