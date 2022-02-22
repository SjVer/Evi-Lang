#include <stdio.h>
#include <stdint.h>

uint8_t printc(char ch)
{
	return putchar(ch);
}

uint8_t printi(int64_t i)
{
	return printf("%ld\n", i);
}

uint8_t printd(double d)
{
	return printf("%g\n", d);
}

uint8_t prints(char* str)
{
	return printf("%s\n", str);
}

uint64_t scani()
{
	int ret;
	scanf("%d", &ret);
	return ret;
}

void printstrarr(uint64_t len, char** arr)
{
	for(int i = 0; i < len; i++)
		prints(arr[i]);
}

void printintarr(uint64_t len, int32_t* arr)
{
	for(int i = 0; i < len; i++)
		printi(arr[i]);
}