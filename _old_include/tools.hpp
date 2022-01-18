#ifndef TOOLS_H
#define TOOLS_H

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// allocates memory for type of size count
#define ALLOCATE(type, count) \
    (type *)reallocate(NULL, sizeof(type) * (count))

// frees smth from a pointer
#define FREE(type, pointer) reallocate(pointer, 0)

// duplicates capacity. (sets to 8 if capacity is 0)
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity)*2)

// frees the memory of the array
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, 0)

// grows array to new size using the type of data and the newCount
#define GROW_ARRAY(type, pointer, newCount) \
    (type *)reallocate(pointer, sizeof(type) * (newCount))

#define ADDR(type, literal) &(type){literal}

// =========== ========= ===========

void *reallocate(void *pointer, size_t newSize);

char *cpystr(const char *chars, int length);
char *strpstr(const char *str, const char *delim);
char *_strpstr(const char *str, const char *delim,
               bool front, bool back);
char *strpstrf(const char *str, const char *delim);
char *strpstrb(const char *str, const char *delim);
char *escstr(const char *str);
char *fstr(const char *format, ...);
bool isnum(const char *str, bool float_allowed);
bool strstart(const char *str, const char* start);
bool strend(const char *str, const char *end);
size_t utf8len(char *s);
char *bitsf(int value, int len);
char *toUpper(const char *str);

int bitlen(uint32_t value);
char *readFile(const char *path);

#endif