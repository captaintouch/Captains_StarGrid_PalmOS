#include "../i_memory.h"

#include <stdlib.h>
#include <string.h>

void *imem_alloc(unsigned long size) {
    return malloc(size);
}

void imem_free(void *ptr) {
    free(ptr);
}

void imem_resize(void *ptr, unsigned long size) {
    /* Mirrors the Palm MemPtrResize contract used by the game: resize in place.
       realloc may move the block, but the game only ever shrinks here, so the
       returned pointer equals the input in practice. */
    realloc(ptr, size);
}

void imem_zero(void *ptr, unsigned long size) {
    memset(ptr, 0, size);
}

void imem_copy(void *dst, void *src, unsigned long size) {
    memmove(dst, src, size);
}
