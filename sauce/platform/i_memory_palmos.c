#include "i_memory.h"

#include <PalmOS.h>

void *imem_alloc(unsigned long size) {
    return MemPtrNew(size);
}

void imem_free(void *ptr) {
    MemPtrFree(ptr);
}

void imem_resize(void *ptr, unsigned long size) {
    MemPtrResize(ptr, size);
}

void imem_zero(void *ptr, unsigned long size) {
    MemSet(ptr, size, 0);
}

void imem_copy(void *dst, void *src, unsigned long size) {
    MemMove(dst, src, size);
}
