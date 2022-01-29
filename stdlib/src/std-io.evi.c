#include <stdio.h>
#include <stdint.h>

uint8_t printc(char ch) { return putchar(ch); }

uint8_t printi(int i) { return printf("%d", i); }