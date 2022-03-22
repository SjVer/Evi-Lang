/*
	Implementation of Evi standard library header "std/io"
	Written by Sjoerd Vermeulen (2022)

	MIT License

	Copyright (c) 2022 Sjoerd Vermeulen

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

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

uint8_t printg(float f)
{
	return printf("%f\n", f);
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