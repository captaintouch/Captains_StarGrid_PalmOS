#ifndef I_MEMORY_H_
#define I_MEMORY_H_

void *imem_alloc(unsigned long size);
void imem_free(void *ptr);
void imem_resize(void *ptr, unsigned long size);
void imem_zero(void *ptr, unsigned long size);
void imem_copy(void *dst, void *src, unsigned long size);

#endif
