#ifndef I_TYPES_H_
#define I_TYPES_H_

/*
 * Platform-neutral primitive types and small helpers.
 *
 * On Palm OS this simply pulls in <PalmOS.h>, so the existing code keeps the
 * exact same types. On any other platform it provides equivalent definitions
 * from the C standard library, so the shared game code can compile without the
 * Palm SDK. Shared headers include this instead of <PalmOS.h>.
 */

#ifdef PALMOS

#include <PalmOS.h>

#else

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* Primitive integer types */
typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef int8_t Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef char Char;

/* Boolean */
typedef unsigned char Boolean;
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

/* Resource / color id aliases used in shared headers */
typedef UInt16 DmResID;
typedef UInt8 IndexedColorType;

/* Geometry types (mirror the Palm SDK layout the shared code relies on) */
typedef Int16 Coord;

typedef struct PointType {
    Coord x;
    Coord y;
} PointType;

typedef struct RectangleType {
    PointType topLeft;
    PointType extent;
} RectangleType;

static inline void RctSetRectangle(RectangleType *rect, Coord x, Coord y, Coord width, Coord height) {
    rect->topLeft.x = x;
    rect->topLeft.y = y;
    rect->extent.x = width;
    rect->extent.y = height;
}

/* String helpers -> C library equivalents (same names to avoid call-site churn) */
#define StrLen(s) ((UInt16)strlen(s))
#define StrCopy(dst, src) strcpy((dst), (src))
#define StrCat(dst, src) strcat((dst), (src))
#define StrIToA(dst, value) sprintf((dst), "%ld", (long)(value))

/* Event-loop delay sentinel used by shared code (Palm's evtWaitForever) */
#ifndef evtWaitForever
#define evtWaitForever (-1)
#endif

#endif /* PALMOS */

#endif /* I_TYPES_H_ */
