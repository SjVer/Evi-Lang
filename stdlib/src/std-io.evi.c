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

uint8_t printd(double d)
{
	return printf("%g", d);
}

uint8_t prints(char* str)
{
	return printf("%s", str);
}

uint64_t scani()
{
	int ret;
	scanf("%d", &ret);
	return ret;
}