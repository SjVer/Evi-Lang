#include <stdio.h>
#include <stdint.h>

uint8_t printc(char ch)
{
	return putchar(ch);
}

uint8_t printi(int64_t i)
{
	return printf("%ld", i);
}

uint64_t scani()
{
	int ret;
	scanf("%d", &ret);
	return ret;
}