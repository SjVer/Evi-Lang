/*
	Implementation of Evi standard library header "std/str"
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

// defined in c's libc:

extern evi_nll_t* memchr(const evi_nll_t*, evi_i32_t, evi_sze_t);

extern evi_i32_t memcmp(const evi_nll_t*, const evi_nll_t*, evi_sze_t);

extern evi_nll_t* memcpy(evi_nll_t*, const evi_nll_t*, evi_sze_t);

extern evi_nll_t* memmove(evi_nll_t*, const evi_nll_t*, evi_sze_t);

extern evi_nll_t* memset(evi_nll_t*, evi_i32_t, evi_sze_t);

extern evi_chr_t* strcat(evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strncat(evi_chr_t*, const evi_chr_t*, evi_sze_t);

extern evi_chr_t* strchr(const evi_chr_t*, evi_i32_t);

extern evi_i32_t strcmp(const evi_chr_t*, const evi_chr_t*);

extern evi_i32_t strncmp(const evi_chr_t*, const evi_chr_t*, evi_sze_t);

extern evi_i32_t strcoll(const evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strcpy(evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strncpy(evi_chr_t*, const evi_chr_t*, evi_sze_t);

extern evi_sze_t strcspn(const evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strerror(evi_i32_t);

extern evi_sze_t strlen(const evi_chr_t*);

extern evi_chr_t* strpbrk(const evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strrchr(const evi_chr_t*, evi_i32_t);

extern evi_sze_t strspn(const evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strstr(const evi_chr_t*, const evi_chr_t*);

extern evi_chr_t* strtok(evi_chr_t*, const evi_chr_t*);

extern evi_sze_t strxfrm(evi_chr_t*, const evi_chr_t*, evi_sze_t);