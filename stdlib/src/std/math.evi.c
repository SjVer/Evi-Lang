/*
	Implementation of Evi standard library header "std/math"
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

// defined in c's libm:

extern evi_dbl_t acos(evi_dbl_t x);

extern evi_dbl_t asin(evi_dbl_t x);

extern evi_dbl_t atan(evi_dbl_t x);

extern evi_dbl_t atan2(evi_dbl_t x, evi_dbl_t y);

extern evi_dbl_t cos(evi_dbl_t x);

extern evi_dbl_t cosh(evi_dbl_t x);

extern evi_dbl_t sin(evi_dbl_t x);

extern evi_dbl_t sinh(evi_dbl_t x);

extern evi_dbl_t tanh(evi_dbl_t x);

extern evi_dbl_t exp(evi_dbl_t x);

extern evi_dbl_t frexp(evi_dbl_t x, evi_i32_t* exponent);

extern evi_dbl_t ldexp(evi_dbl_t x, evi_i32_t exponent);

extern evi_dbl_t log(evi_dbl_t x);

extern evi_dbl_t log10(evi_dbl_t x);

extern evi_dbl_t modf(evi_dbl_t x, evi_dbl_t* integer);

extern evi_dbl_t pow(evi_dbl_t x, evi_dbl_t y);

extern evi_dbl_t sqrt(evi_dbl_t x);

extern evi_dbl_t ceil(evi_dbl_t x);

extern evi_dbl_t fabs(evi_dbl_t x);

extern evi_dbl_t floor(evi_dbl_t x);

extern evi_dbl_t fmod(evi_dbl_t x, evi_dbl_t y);