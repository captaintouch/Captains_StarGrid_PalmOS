#include "i_video.h"

#include <PalmOS.h>

IVideoBuffer *ivideo_createBuffer(int width, int height) {
    Err err = errNone;
    WinHandle handle = WinCreateOffscreenWindow(width, height, nativeFormat, &err);
    if (err != errNone) {
        return NULL;
    }
    return (IVideoBuffer *)handle;
}

void ivideo_deleteBuffer(IVideoBuffer *buffer) {
    if (buffer != NULL) {
        WinDeleteWindow((WinHandle)buffer, false);
    }
}

void ivideo_setDrawTarget(IVideoBuffer *buffer) {
    WinSetDrawWindow(buffer != NULL ? (WinHandle)buffer : WinGetDisplayWindow());
}

IVideoBuffer *ivideo_mainScreenBuffer() {
    return (IVideoBuffer *)WinGetDisplayWindow();
}

void ivideo_copyRect(IVideoBuffer *src, IVideoBuffer *dst, Coordinate srcOrigin, Coordinate size, Coordinate dstOrigin) {
    RectangleType rect;
    RctSetRectangle(&rect, srcOrigin.x, srcOrigin.y, size.x, size.y);
    WinCopyRectangle((WinHandle)src, (WinHandle)dst, &rect, dstOrigin.x, dstOrigin.y, winPaint);
}
