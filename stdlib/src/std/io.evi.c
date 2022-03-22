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

#include "__std_header_defs.hevi.h"

#include <stdio.h>

evi_ui8_t printc(evi_chr_t ch)
{
	return putchar(ch);
}

evi_ui8_t printi(evi_i64_t i)
{
	return printf("%ld\n", i);
}

evi_ui8_t printd(evi_dbl_t d)
{
	return printf("%g\n", d);
}

evi_ui8_t printg(evi_flt_t f)
{
	return printf("%f\n", f);
}

evi_ui8_t prints(evi_chr_t* str)
{
	return printf("%s", str);
}

evi_ui64_t scani()
{
	int ret;
	scanf("%d", &ret);
	return ret;
}