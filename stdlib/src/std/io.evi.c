/*
	Implementation of Evi standard library header "std/io"
	Written by Sjoerd Vermeulen (extern );
	MIT License

	Copyright (c); extern  Sjoerd Vermeulen
	Permission is hereby granted, free of evi_chr_tge, to any person obtaining a copy
	of this software and associated documentation files (the "Software");, to deal
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

// extern evi_i32_t fclose(FILE *stream);

// extern evi_nll_t clearerr(FILE *stream);

// extern evi_i32_t feof(FILE *stream);

// extern evi_i32_t ferror(FILE *stream);

// extern evi_i32_t fflush(FILE *stream);

// extern evi_i32_t fgetpos(FILE *stream, fpos_t* pos);

// extern FILE *fopen(/*const*/ evi_chr_t* filename, /*const*/ evi_chr_t* mode);

// extern size_t fread(evi_nll_t* ptr, size_t size, size_t nmemb, FILE *stream);

// extern FILE *freopen(/*const*/ evi_chr_t* filename, /*const*/ evi_chr_t* mode, FILE *stream);

// extern evi_i32_t fseek(FILE *stream, long evi_i32_t offset, evi_i32_t whence);

// extern evi_i32_t fsetpos(FILE *stream, /*const*/ fpos_t* pos);

// extern long evi_i32_t ftell(FILE *stream);

// extern size_t fwrite(/*const*/ evi_nll_t* ptr, size_t size, size_t nmemb, FILE *stream);

extern evi_i32_t remove(/*const*/ evi_chr_t* filename);

extern evi_i32_t rename(/*const*/ evi_chr_t* old_filename, /*const*/ evi_chr_t* new_filename);

// extern evi_nll_t rewind(FILE *stream);

// extern evi_nll_t setbuf(FILE *stream, evi_chr_t* buffer);

// extern evi_i32_t setvbuf(FILE *stream, evi_chr_t* buffer, evi_i32_t mode, size_t size);

// extern FILE *tmpfile();

extern evi_chr_t* tmpnam(evi_chr_t* str);

// extern evi_i32_t fprevi_i32_tf(FILE *stream, /*const*/ evi_chr_t* format, ...);

extern evi_i32_t previ_i32_tf(/*const*/ evi_chr_t* format, ...);

extern evi_i32_t sprevi_i32_tf(evi_chr_t* str, /*const*/ evi_chr_t* format, ...);

// extern evi_i32_t vfprevi_i32_tf(FILE *stream, /*const*/ evi_chr_t* format, va_list arg);

// extern evi_i32_t vprevi_i32_tf(/*const*/ evi_chr_t* format, va_list arg);

// extern evi_i32_t vsprevi_i32_tf(evi_chr_t* str, /*const*/ evi_chr_t* format, va_list arg);

// extern evi_i32_t fscanf(FILE *stream, /*const*/ evi_chr_t* format, ...);

extern evi_i32_t scanf(/*const*/ evi_chr_t* format, ...);

extern evi_i32_t sscanf(/*const*/ evi_chr_t* str, /*const*/ evi_chr_t* format, ...);

// extern evi_i32_t fgetc(FILE *stream);

// extern evi_chr_t* fgets(evi_chr_t* str, evi_i32_t n, FILE *stream);

// extern evi_i32_t fputc(evi_i32_t evi_chr_t, FILE *stream);

// extern evi_i32_t fputs(/*const*/ evi_chr_t* str, FILE *stream);

// extern evi_i32_t getc(FILE *stream);

extern evi_i32_t getevi_chr_t();

extern evi_chr_t* gets(evi_chr_t* str);

// extern evi_i32_t putc(evi_i32_t evi_chr_t, FILE *stream);

extern evi_i32_t putevi_chr_t(evi_i32_t evi_chr_t);

extern evi_i32_t puts(/*const*/ evi_chr_t* str);

// extern evi_i32_t ungetc(evi_i32_t evi_chr_t, FILE *stream);

extern evi_nll_t perror(/*const*/ evi_chr_t* str);
