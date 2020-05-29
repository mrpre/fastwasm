#pragma once

#include <malloc.h>
#include <string.h>
static inline void fw_free(void *ptr)
{
    //printf("*******fw_free %p\n", ptr);
    free(ptr);
}

static inline void *fw_calloc(size_t size)
{
    void *ptr  = calloc(1, size);

    //printf("*******malloc %p %zd\n", ptr, size);
    return ptr;
}
static inline void *fw_malloc(size_t size)
{
    void *ptr  =  malloc(size);

    //printf("*******malloc %p %zd\n", ptr, size);
    return ptr;
}

static inline void *fw_cpymem(void *dest, const void *src, size_t n)
{
    memcpy(dest, src, n);
    return (char*)dest + n;
}

static inline void *fw_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

static inline void *fw_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}
