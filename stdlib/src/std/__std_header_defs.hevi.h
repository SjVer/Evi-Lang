/*
	Implementation of Evi standard library header "std/__std_header_defs"
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

// ============= C version of Evi's types =============

typedef /*signed*/ _Bool		evi_i1_t;		// i1

typedef signed char	/*?*/		evi_i4_t;		// i4
typedef unsigned char /*?*/		evi_ui4_t;		// ui4

typedef signed char				evi_i8_t;		// i8
typedef unsigned char			evi_ui8_t;		// ui8

typedef signed short int		evi_i16_t;		// i16
typedef unsigned short int		evi_ui16_t;		// ui16

typedef signed int				evi_i32_t;		// i32
typedef unsigned int			evi_ui32_t;		// ui32

typedef signed long int			evi_i64_t;		// i64
typedef unsigned long int		evi_ui64_t;		// ui64

typedef __int128_t				evi_i128_t;		// i128
typedef __uint128_t				evi_ui128_t;	// ui128

typedef float					evi_flt_t;		// flt
typedef double					evi_dbl_t;		// dbl

typedef unsigned long 			evi_sze_t;		// sze
typedef /*unsigned*/ _Bool		evi_bln_t;		// bln
typedef char 					evi_chr_t;		// chr
typedef void 					evi_nll_t;		// nll

// ====================================================