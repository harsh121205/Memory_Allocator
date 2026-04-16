#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>


void *malloc_track(size_t size, const char *file, int line);
void free(void *ptr);
void *calloc_track(size_t num, size_t size, const char *file, int line);
void *realloc_track(void *ptr, size_t size, const char *file, int line);
void print_heap_dump();


#define malloc(size)       malloc_track(size, __FILE__, __LINE__)
#define free(ptr)          free(ptr)
#define calloc(num, size)  calloc_track(num, size, __FILE__, __LINE__)
#define realloc(ptr, size) realloc_track(ptr, size, __FILE__, __LINE__)

#endif